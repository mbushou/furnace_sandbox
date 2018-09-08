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
import gc
import cProfile
import timeit


class App(object):
    def __init__(self, ctx):
        fe.log("in pslist constructor")
        fe.set_name("pslist")

        self.lookups = {
            "name_offset": fe.lookup_structure("task_struct", "comm"),
            "pid_offset": fe.lookup_structure("task_struct", "pid"),
            "tasks_offset": fe.lookup_structure("task_struct", "tasks"),
            "init_task": fe.lookup_symbol("init_task"),
        }

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        e = {
            "event_type": fe.TIMER,
            "time_value": 0.1,  # seconds
            "callback": self.timer_callback,
        }
        fe.event_register(e)

        self.pslist = {}
        self.log = []
        self.calls = []
        self.msgs = []
        self.procs = []

        fe.log("leaving pslist constructor")

    def timer_callback(self, ctx):

        timeit_on = False
        cprof = False
        once = True

        if cprof:
            self.do_pslist(pause=True)  # zmq warmup
            cProfile.runctx(
                "self.do_pslist()",
                globals(),
                locals(),
                filename="/home/micah/femain.single.profile",
            )

        if timeit_on:
            fe.log("warming up")
            self.do_pslist(pause=True)  # zmq warmup
            fe.pause_vm()
            fe.log("running timeit")
            t = timeit.Timer(lambda: self.do_pslist()).timeit(number=100)
            fe.log("done timeit")
            t = t / 100.0
            fe.resume_vm()
            fe.log("timeit pslist in {sec:.6f} seconds".format(sec=t))

        if once:
            gc.disable()
            self.do_pslist(pause=True)
            gc.enable()
            fe.log(self.pslist)

        for l in self.log:
            fe.log(l)
        # fe.log(self.pslist)
        fe.log("calls: %s" % self.calls)
        fe.log("procs: %s" % self.procs)
        fe.exit()

    def do_pslist(self, pause=False):
        if pause:
            fe.pause_vm()

        calls = 0
        procs = 0

        start_pslist = time.time()
        list_head = self.lookups["init_task"] + self.lookups["tasks_offset"]
        cur_list_entry = list_head
        next_list_entry = fe.read_addr_va(list_head, 0)
        calls += 1

        i = 0
        maxi = 100
        # fe.log('starting PSLIST')
        while True:
            cur_proc = cur_list_entry - self.lookups["tasks_offset"]

            pid = fe.read_32_va(cur_proc + self.lookups["pid_offset"], 0)
            calls += 1
            procname = fe.read_str_va(cur_proc + self.lookups["name_offset"], 0)
            calls += 1
            # fe.log('%5s %-16s' % (pid, procname))
            self.pslist[cur_proc] = (pid, procname)
            cur_list_entry = next_list_entry
            next_list_entry = fe.read_addr_va(cur_list_entry, 0)
            calls += 1
            i += 1
            # fe.log('%s == %s?' % (hex(cur_list_entry), hex(list_head)))
            if cur_list_entry & 0xffffffffffff == list_head or i > maxi:
                # fe.log('ending loop at %d' % (i,))
                end_pslist = time.time()
                self.log.append(end_pslist - start_pslist)
                break
            procs += 1

        self.calls.append(calls)
        self.procs.append(procs)
        if pause:
            fe.resume_vm()
        # fe.log('pslist in {sec:.6f} seconds'.format(sec=stop_pslist-start_pslist))

    def be_callback(self, ctx):
        fe.log("{ctr:<3} BE CALLBACK".format(ctr=self.cr3_ctr))
        fe.log("    passed: {ctx}".format(ctx=ctx))
