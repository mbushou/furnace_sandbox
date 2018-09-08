# -------------------------
# Furnace (c) 2017-2018 Micah Bushouse
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -------------------------
import fe
import time
import json

INIT = 0
INJECTED = 1
TRACING = 2


class App(object):
    def __init__(self, ctx):
        fe.log("in arav constructor")
        fe.set_name("arav")

        self.l = {
            "name_offset": fe.lookup_structure("task_struct", "comm"),
            "pid_offset": fe.lookup_structure("task_struct", "pid"),
            "tasks_offset": fe.lookup_structure("task_struct", "tasks"),
            "mm_offset": fe.lookup_structure("task_struct", "mm"),
            "pgd_offset": fe.lookup_structure("mm_struct", "pgd"),
            "vm_file_offset": fe.lookup_structure("vm_area_struct", "vm_file"),
            "vm_file_path_offset": fe.lookup_structure("file", "f_path"),
            "path_dentry_offset": fe.lookup_structure("path", "dentry"),
            "dentry_d_name_offset": fe.lookup_structure("dentry", "d_name"),
            "dentry_d_parent_offset": fe.lookup_structure("dentry", "d_parent"),
            "d_name_str_offset": fe.lookup_structure("qstr", "name"),
            "commit_creds": fe.lookup_symbol("commit_creds"),
        }

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        e = {"event_type": fe.TIMER, "time_value": 2.0, "callback": self.timer_callback}
        fe.event_register(e)

        self.state = INIT
        self.pslist = {}
        self.events = {}
        self.target_proc = {}
        self.last = 0.0
        self.tick = 0
        self.symbol_string = ""
        self.symbols = {}
        self.fmts = []
        self.users = []

        self.load_symbols()
        self.parse_symbols()
        fe.events_start()

        fe.log("leaving arav constructor")

    def timer_callback(self, ctx):

        now = time.time()
        if self.last + 10 < now:
            fe.log("refreshing pslist")
            # pslist[proc.pid] = {'name': proc.name, 'process_block_ptr': proc.process_block_ptr}
            self.pslist = fe.process_list()
            # fe.log(f'#procs: {len(self.pslist)}')

            for pid, proc in self.pslist.items():
                if proc["name"] == "sshd":
                    fe.log(f"found sshd: {pid} {proc}")
                    if not self.target_proc:
                        self.target_proc["pid"] = pid
                        for k, v in proc.items():
                            self.target_proc[k] = v
                        for k, v in self.enumerate_proc(
                            proc["process_block_ptr"]
                        ).items():
                            self.target_proc[k] = v
                        fe.log(f"loading target_proc {self.target_proc}")

        if self.state == INIT:
            fe.log(f"installing breakpoint on commit_creds")
            address = self.l["commit_creds"]
            e = {
                "event_type": fe.INT,
                "sync": fe.SYNC,
                "bp_pid": fe.KERNEL,
                "bp_addr": address,
                "callback": self.commit_creds_callback,
            }
            try:
                eid = fe.event_register(e)
            except Exception:
                fe.log(f"could not inject at commit_creds")
                errors.append("commit_creds")
            else:
                self.events[eid] = address
                fe.log(f"event is {eid}")
                self.symbols[address] = {
                    "address": address,
                    "t": "T",
                    "func_name": "commit_creds",
                }
            self.state = INJECTED

        if False and self.state == INIT:
            fe.log(f"injecting against {self.target_proc}")
            self.state = INJECTED
            errors = []
            for address, info in self.symbols.items():
                if "do_log" != info["func_name"]:
                    fe.log(f"skipping {info['func_name']}")
                    continue
                bp_addr = self.target_proc["text_start"] + address
                pid = self.target_proc["pid"]
                fe.log(f"installing bp at {bp_addr:x} for {address}:{info}")
                e = {
                    "event_type": fe.INT,
                    "sync": fe.SYNC,
                    "bp_pid": pid,
                    "bp_addr": bp_addr,
                    "callback": self.int_callback,
                }
                try:
                    eid = fe.event_register(e)
                except Exception:
                    fe.log(f"could not inject at {info['func_name']}")
                    errors.append(info["func_name"])
                else:
                    self.events[eid] = address
                    fe.log(f"event is {eid}")
            fe.log(f"done, errors: {errors}")

        if self.tick == 15:
            fe.log("shutting down")
            fe.events_stop()
            for eid, address in self.events.items():
                fe.log(f"clearing {eid}: {self.symbols[address]['func_name']}")
                fe.event_clear(eid)
            self.events = {}
            print(f"formats: {self.fmts}")
            print(f"users: {self.users}")

            fe.exit()

        fe.log(f"tick {self.tick}")
        self.tick += 1
        self.last = now

    def be_callback(self, ctx):
        fe.log(f"be callback: {ctx}")

    def commit_creds_callback(self, ctx):
        fe.log(f"commit_creds: {ctx}")
        fe.notify(json.dumps({"cmd": "commit_creds", "data": ctx._asdict()}))

    def int_callback(self, ctx):
        self.state = TRACING
        address = self.events[ctx.identifier]
        info = self.symbols[address]
        func_name = info["func_name"]
        pid = self.target_proc["pid"]
        fe.log(f"int callback: {ctx}")
        fe.log(f"called: {func_name}")
        if pid != ctx.pid:
            fe.log(f"wrong pid {ctx.pid} called us!")
        if func_name == "do_log":
            fmt = fe.read_str_va(ctx.rsi, ctx.pid)
            fe.log(f"do_log fmt: {fmt}")
            self.fmts.append(fmt)
            if "userauth-request" in fmt:
                fe.log("userauth-request, searching for username...")
                user = fe.read_str_va(ctx.rdx, ctx.pid)
                fe.log(f"got {user}")
                self.users.append(user)
                """
            elif 'Starting session' in fmt:
                fe.log('starting session, searching for username...')
                user = fe.read_str_va(ctx.rdx, ctx.pid)  # need r9....
                fe.log(f'got {user}')
                self.users.append(user)
"""

    def enumerate_proc(self, current_process):
        res = {}
        mm_ptr = fe.read_addr_va(current_process + self.l["mm_offset"], 0)
        fe.log(f"mm_ptr: {mm_ptr}")
        if mm_ptr:
            task_pgd_va = fe.read_addr_va(mm_ptr + self.l["pgd_offset"], 0)
            fe.log(f"task_pgd_va: {task_pgd_va}")
            task_pgd = fe.translate_kv2p(task_pgd_va)
            fe.log(f"task_pgd: {task_pgd}")
            # parent_process = fe.read_addr_va(current_process + self.l['parent'], 0)
            # pprocname = fe.read_str_va(parent_process + self.l['name_offset'], 0)
            # ppid = fe.read_32_va(parent_process + self.l['pid_offset'], 0)
            # map_count = fe.read_64_va(current_process + self.l['map_count'], 0)
            # total_vm = fe.read_64_va(current_process + self.l['total_vm'], 0)
            # exec_vm = fe.read_64_va(current_process + self.l['exec_vm'], 0)
            # stack_vm = fe.read_64_va(current_process + self.l['stack_vm'], 0)
            # cred = fe.read_addr_va(current_process + self.l['cred'], 0)
            # uid = fe.read_32_va(cred + self.l['uid'], 0)
            # cputime = fe.read_64_va(current_process + self.l['sched_entity'] + self.l['sum_exec_runtime'], 0)
            vma_head_ptr = fe.read_addr_va(mm_ptr, fe.KERNEL)
            fe.log(f"vma_head_ptr: {vma_head_ptr}")
            fullpath = self.get_vma_file(vma_head_ptr)
            fe.log(f"fullpath: {fullpath}")
            start = fe.read_addr_va(vma_head_ptr + 0, 0)  # vm_area_struct: vm_start
            end = fe.read_addr_va(vma_head_ptr + 8, 0)  # vm_area_struct: vm_end
            mapped_size = end - start
            fe.log(f"start: {start:x}, size: {mapped_size:x}")
        res = {
            "mm_ptr": mm_ptr,
            "task_pgd_va": task_pgd_va,
            "vma_head_ptr": vma_head_ptr,
            "fullpath": fullpath,
            "text_start": start,
            "text_end": end,
        }
        return res

    def get_vma_file(self, current_vma):
        path = []
        path_str = ""
        #  vma -> *file -> path -> *dentry -> qstr -> *char
        file_p = fe.read_addr_va(current_vma + self.l["vm_file_offset"], 0)
        if file_p == 0:
            # This is an anonymous region.
            pass
        else:
            dentry_p = fe.read_addr_va(
                file_p + self.l["vm_file_path_offset"] + self.l["path_dentry_offset"], 0
            )
            i = 0
            ml = 20
            while True:
                char_p = fe.read_addr_va(
                    dentry_p
                    + self.l["dentry_d_name_offset"]
                    + self.l["d_name_str_offset"],
                    0,
                )
                if char_p == 0:
                    fe.log("char_p null")
                    vm_file = ""
                else:
                    vm_file = fe.read_str_va(char_p, 0)
                    fe.log(vm_file)

                if vm_file == "/":
                    path_str = ""
                    for i in path[::-1]:
                        path_str = "%s/%s" % (path_str, i)
                    break
                else:
                    path.append(vm_file)

                i += 1
                if i > ml:
                    break

                dentry_p = fe.read_addr_va(
                    dentry_p + self.l["dentry_d_parent_offset"], 0
                )

        return path_str

    def parse_symbols(self):
        self.symbols = {}
        for l in self.symbol_string.split("\n"):
            f1, t, func_name = l.split()
            address = int(f1, 16)
            self.symbols[address] = {"address": address, "t": t, "func_name": func_name}
            fe.log(f"{address}: {func_name}")
        fe.log(f"parsed {len(self.symbols)} symbols")

    def load_symbols(self):
        # cat ~/third/arav/sshd.symbols |grep " [t|T] " |grep -e pam -e log
        self.symbol_string = """0000000000011150 t linux_audit_user_logxxx
0000000000012610 t store_lastlog_message.isra.0
00000000000127b0 T get_last_login_time
0000000000012820 t record_login
0000000000012890 t record_logout
000000000001be40 t auth_log
000000000001ecf0 t display_loginmsg
000000000001f2d0 T check_quietlogin
0000000000020d00 T do_login
0000000000028e00 T mm_answer_pam_free_ctx
00000000000299a0 T mm_answer_pam_init_ctx
0000000000029b90 T mm_answer_pam_query
0000000000029d90 T mm_answer_pam_respond
000000000002b890 t monitor_read_log.isra.0
000000000002ba80 T mm_answer_pam_start
000000000002baf0 T mm_answer_pam_account
000000000002d0e0 T mm_log_handler
000000000002e4a0 t mm_start_pam
000000000002e540 t mm_do_pam_account
000000000002e650 T mm_sshpam_init_ctx
000000000002e730 T mm_sshpam_query
000000000002e8f0 T mm_sshpam_respond
000000000002e9f0 T mm_sshpam_free_ctx
000000000002fe40 t ssh_krb5_get_k5login_directory
0000000000031e90 t ssh_gssapi_k5login_exists
00000000000332d0 t lastlog_openseek
0000000000033500 t login_free_entry
0000000000033510 T login_set_current_time
0000000000033570 t login_set_addr
00000000000336a0 T login_init_entry
0000000000033770 t login_alloc_entry
0000000000033810 t lastlog_write_entry.part.2
0000000000033b70 T syslogin_write_entry
0000000000033c50 T lastlog_write_entry
0000000000033c80 T login_write
0000000000033d50 t login_login
0000000000033d60 t login_logout
0000000000033d70 T lastlog_get_entry
0000000000033ed0 t login_get_lastlog
0000000000033fb0 T login_get_lastlog_time
0000000000034010 t record_failed_login
00000000000342a0 t sshpam_null_conv
00000000000343c0 t sshpam_init
0000000000034550 t sshpam_respond
00000000000346b0 t sshpam_store_conv
00000000000347d0 t sshpam_thread_conv
00000000000349d0 t sshpam_sigchld_handler
0000000000034aa0 t sshpam_tty_conv.part.7
0000000000034c50 t sshpam_tty_conv
0000000000034cb0 t sshpam_passwd_conv
0000000000034e20 t sshpam_set_maxtries_reached.part.12
0000000000034e50 T sshpam_password_change_required
0000000000034ed0 t sshpam_query
00000000000354e0 t sshpam_thread_cleanup
0000000000035600 t sshpam_free_ctx
0000000000035630 t sshpam_cleanup
0000000000035730 t start_pam
0000000000035770 t finish_pam
0000000000035780 t do_pam_account
0000000000035840 T do_pam_set_tty
00000000000358a0 t do_pam_setcred
0000000000035990 t do_pam_chauthtok
0000000000035a40 t do_pam_session
0000000000035b00 t is_pam_session_open
0000000000035b10 t do_pam_putenv
0000000000035ba0 t sshpam_init_ctx
0000000000036150 t fetch_pam_child_environment
0000000000036160 t fetch_pam_environment
0000000000036170 t free_pam_environment
00000000000361c0 t sshpam_auth_passwd
00000000000363a0 t sshpam_get_maxtries_reached
00000000000363b0 t sshpam_set_maxtries_reached
0000000000038b70 t handle_log_close
0000000000052660 t log_facility_number
00000000000526e0 t log_facility_name
0000000000052720 t log_level_number
00000000000527a0 t log_level_name
00000000000527f0 t log_init_handler
00000000000529f0 t log_init
0000000000052a00 T log_change_level
0000000000052a40 T log_is_on_stderr
0000000000052a60 t log_redirect_stderr_to
0000000000052ac0 t set_log_handler
0000000000052ad0 t do_log
0000000000052eb0 t logdie
0000000000052f60 t logit
0000000000053320 t do_log2
0000000000079e50 t setlogin"""
