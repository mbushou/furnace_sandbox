/*
 * Furnace (c) 2017-2018 Micah Bushouse
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma GCC diagnostic ignored "-Wunused-result"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>

#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <libexplain/waitpid.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/unistd.h>
#include <libgen.h>

#include <map>

#include <seccomp.h>

#define DEBUG

#ifdef DEBUG
#define PRINT_DEBUG(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define PRINT_DEBUG(fmt, args...)
#endif

// Attribution: An early version of this source was inspired from:
//http://www.alfonsobeato.net/c/modifying-system-call-arguments-with-ptrace/
//http://www.alfonsobeato.net/c/filter-and-modify-system-calls-with-seccomp-and-ptrace/

// Globals
int c = 0;  // outer loop
int firstjump = 0;
std::map<pid_t, int> pids;
pid_t last_pid = 0;


static void read_reg_string(int reg, pid_t child, char* file)   // *file is allocated on stack
{
    char* child_addr;
    unsigned int i;
    child_addr = (char*) ptrace(PTRACE_PEEKUSER, child, sizeof(long)*reg, 0);
    do
    {
        long val;
        char* p;
        val = ptrace(PTRACE_PEEKTEXT, child, child_addr, NULL);
        if (val == -1)
        {
            //error: %s", strerror(errno)
            perror("PT: PTRACE_PEEKTEXT error: ");
            exit(1);
        }
        child_addr += sizeof(long);
        p = (char*) &val;
        for (i = 0; i < sizeof(long); ++i, ++file)
        {
            *file = *p++;
            if (*file == '\0') break;
        }
    }
    while (i == sizeof (long));
}


static int read_reg_int(int reg, pid_t child)   // *file is allocated on stack
{
    return ptrace(PTRACE_PEEKUSER, child, sizeof(long)*reg, 0);
}


// returns 0 if everything's ok, else the return code of the exited process
// + 128
int read_waitpid_status(int status)
{
    int ret = 0;
    if (WIFEXITED(status))
    {
        PRINT_DEBUG("PT: child exited with rc=%d\n", WEXITSTATUS(status));
        ret = WEXITSTATUS(status) + 128;
    }
    if (WIFSIGNALED(status))
    {
        // 31 == SIGSYS
        char* strsig = strsignal(WTERMSIG(status));
        PRINT_DEBUG("PT: child exited via signal %s\n", strsig);
        PRINT_DEBUG("PT: core dump? %d\n", WCOREDUMP(status));
    }
    if (WIFCONTINUED(status))
    {
        PRINT_DEBUG("PT: child process was resumed\n");
    }
    return ret;
}


void build_syscall_libseccomp_filter(scmp_filter_ctx* ctx)
{
    int rc;
    *ctx = seccomp_init(SCMP_ACT_KILL);
    if (*ctx == NULL)
    {
        printf("ctx is null\n");
        return;
    }
    rc = seccomp_attr_set(*ctx, SCMP_FLTATR_CTL_TSYNC, 1);
    if (rc != 0)
    {
        printf("could not set tsync\n");
    }
    rc = seccomp_attr_set(*ctx, SCMP_FLTATR_CTL_NNP, 1);
    if (rc != 0)
    {
        printf("could not set no new privs\n");
    }

    rc = 0;

    // BEGIN Perm whitelisted syscalls
    // Example: rc |= seccomp_rule_add(*ctx, SCMP_ACT_ALLOW, SCMP_SYS(poll), 0);
//ALLOW_LIST
    // END Perm whitelisted syscalls

    if (rc != 0)
    {
        printf("could not set rules 1 rc=%d\n", rc);
    }
    rc = 0;

    // BEGIN Traced syscalls
    // Example: rc |= seccomp_rule_add(*ctx, SCMP_ACT_TRACE(2), SCMP_SYS(execve), 0);
//TRACED_LIST
    // END  Traced syscalls

    if (rc != 0)
    {
        printf("could not set seccomp rules rc=%d\n", rc);
    }
    return;
}


int main(int argc, char** argv)
{
    pid_t pid;
    int status;
    int rc = 0;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <prog> <arg1> ... <argN>\n", argv[0]);
        return 1;
    }

    pid = fork();

    if (!pid)    // gets run by child only
    {
        PRINT_DEBUG("CHILD: hello\n");
        //inside_pid = getpid();  // child's
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        scmp_filter_ctx ctx;
        build_syscall_libseccomp_filter(&ctx);
        PRINT_DEBUG("CHILD: loading seccomp\n");
        fflush(stdout);
        rc = seccomp_load(ctx);
        seccomp_release(ctx);
        if (rc != 0)
        {
            perror("CHILD: could not load rules: ");
            return -1;
        }
        fflush(stdout);
        PRINT_DEBUG("CHILD: done loading seccomp\n");
        PRINT_DEBUG("CHILD: calling SIGSTOP\n");
        kill(getpid(), SIGSTOP);
        PRINT_DEBUG("CHILD: resumed, calling execvp\n");
        fflush(stdout);
        execvp(argv[1], argv + 1);  // this generates a SIGSTOP per manpage
        return -1;  // this will never get called, child execution stops here
    }

    // PARENT TRACER
    pids[pid] = 0;
    PRINT_DEBUG("PT: child pid=%d\n", pid);
    last_pid = pid;

    PRINT_DEBUG("PT: entering first waitpid\n");
    waitpid(pid, &status, 0);  // this catches that initial SIGSTOP?

    PRINT_DEBUG("PT: first waitpid status: 0x%08x, pid:%d = status:%d\n", status, pid, status);
    read_waitpid_status(status);

    int trace_opts = PTRACE_O_TRACESECCOMP | PTRACE_O_EXITKILL
                     | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEVFORK
                     | PTRACE_O_TRACEFORK;

    rc = ptrace(PTRACE_SETOPTIONS, pid, 0, trace_opts);
    if (rc != 0)
    {
        perror("PT: PTRACE error: ");
    }
    int jmax = 0;
    int policy_level = 0;
    std::map<uint64_t, int> policy_map;

    // event loop for parent
    while (1)
    {
        int status = 0;
        pid_t pid = 0;
        uint64_t sysno = 0;
        //uint64_t reg = 0;
        int rc = 0;
        c++;
        char orig_file[PATH_MAX];
        char* path;
        char* base;
        int options = __WALL;

        ptrace(PTRACE_CONT, last_pid, 0, 0);  // waitpid blocks

        //PRINT_DEBUG("PT: blocking on waitpid\n");
        pid = waitpid(-1, &status, options);  // __WALL wait regardless of type

        last_pid = pid;
        //pid = waitpid(child, &status, __WALL);  // __WALL wait regardless of type
        PRINT_DEBUG("PT: %d [wp: 0x%08x, pid:%d = status:%d] ", c, status, pid, status);

        if (last_pid < 0)
        {
            printf("%s\n", explain_waitpid(pid, &status, options));
            exit(EXIT_FAILURE);
        }

        rc = read_waitpid_status(status);
        if (rc != 0)
        {
            PRINT_DEBUG("PT: a child (%d) exited with code %d (j=%d)\n", last_pid, rc!=0 ? rc - 128 : rc, jmax);
            jmax += 1;
            if (jmax > 10)
            {
                printf("PT: jmax exceeded, exiting...\n");
                return 0;
            }
        }

        if (pids.find(pid) == pids.end())
        {
            PRINT_DEBUG("PT: new pid!\n");
            pids[pid] = 0;
        }
        else
        {
            pids[pid]++;
        }

        // process ptrace events
        if (status >> 8 & SIGTRAP)
        {
            PRINT_DEBUG("trap: ");

            if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8)))
            {
                PRINT_DEBUG("SECCOMP ");
                sysno = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*ORIG_RAX, 0);

                // front end filter, allowed syscalls must be in this map
                if (policy_level == 1 && policy_map.find(sysno) == policy_map.end())
                {
                    PRINT_DEBUG("PID %d: %s(%lu), disallowed\n", pid, seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, sysno), sysno);
                    return 0;
                }

                switch (sysno)
                {
                    case __NR_open:
                        read_reg_string(RDI, pid, orig_file);
                        PRINT_DEBUG("%d [Opening '%s']\n", c, orig_file);

                        if (policy_level == 1)
                        {
                            path = dirname(orig_file);
                            base = basename(orig_file);

                            // BEGIN open list
                            // Example: if (strcmp("/opt/furnace/frontend/apps", path) == 0) break;
//OPEN_LIST
                            // END open list

                            PRINT_DEBUG("PID %d: %s(%lu): %s disallowed\n", pid, seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, sysno), sysno, orig_file);
                            return 0;
                        }

                        if (policy_level == 0 && strcmp("ESTABLISHED", orig_file) == 0)
                        {
                            PRINT_DEBUG("PT: current policy_level: %d\n", policy_level);
                            PRINT_DEBUG("PT: ESTABLISHED signal detected\n");
                            policy_level = 1;
                            PRINT_DEBUG("PT: new policy_level: %d\n", policy_level);

                            // BEGIN policy map
                            // Example: policy_map[__NR_stat] = 1;
//POLICY_MAP_LIST
                            // END policy map
                        }
                        break;

                    // BEGIN traced switch statements
                    /* Example
                    * case __NR_execve:
                     *   read_reg_string(RDI, pid, orig_file);
                     *   PRINT_DEBUG("[execve %s]\n", orig_file);
                     *   break;
                    */
//TRACED_LIST_SWITCH
                    // END traced switch statements


                    // BEGIN test statements
                    /* case __NR_ioctl:
                     *    PRINT_DEBUG("[ioctl %d]\n", read_reg_int(RSI, pid));
                     *    switch (pid)
                     *    {
                     *        case 21585: break;
                     *        case 21505: break;
                     *        default:
                     *            PRINT_DEBUG("ioctl RSI disallowed!\n");
                     *            return 0;
                     *    }
                     *    break;
                    */
//TEST_LIST
                    // END test statements

                    default:
                        PRINT_DEBUG("%s(%lu), no handler\n", seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, sysno), sysno);
                        break;
                }
            }
            else if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8)) ||
                     status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8)) ||
                     status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK << 8)) ||
                     status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK << 8)) )
            {

                PRINT_DEBUG("PT: CLONE* ");
                PRINT_DEBUG("C:%d ", status >> 8 == (SIGTRAP | (PTRACE_EVENT_CLONE << 8)));
                PRINT_DEBUG("E:%d ", status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8)));
                PRINT_DEBUG("V:%d ", status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK << 8)));
                PRINT_DEBUG("F:%d", status >> 8 == (SIGTRAP | (PTRACE_EVENT_FORK << 8)));

                firstjump = 0;
            }
            else
            {
                printf("PT: unknown waitpid status symbol\n");
                printf("VFORK_DONE:%d, EVENT_EXIT:%d\n",
                       status >> 8 == (SIGTRAP | (PTRACE_EVENT_VFORK_DONE << 8)),
                       status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXIT << 8)));
                read_waitpid_status(status);
                if (firstjump == 0)
                {
                    firstjump = 1;
                    printf("PT: firstjump\n");
                }
            }
        }
    }
    return 0;
}
