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


class App(object):
    def __init__(self, ctx):
        fe.log("in vmi_runtime_testing constructor")
        fe.event_register(
            {"event_type": fe.TIMER, "time_value": 1.0, "callback": self.timer_callback}
        )

        self.lookups = {
            "name_offset": fe.get_offset("linux_name"),
            "pid_offset": fe.get_offset("linux_pid"),
            "tasks_offset": fe.get_offset("linux_tasks"),
            "init_task": fe.translate_ksym2v("init_task"),
        }

        self.pslist = {}

    def timer_callback(self, ctx):
        fe.log("calling functions!")

        # e = fe.translate_ksym2v('init_task')
        # fe.log(f'TRANSLATE_KSYM2V init_task: {e}')

        self.do_pslist(pause=True)

        fe.log("done!")
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
        fe.log("starting PSLIST")
        while True:
            cur_proc = cur_list_entry - self.lookups["tasks_offset"]

            pid = fe.read_32_va(cur_proc + self.lookups["pid_offset"], 0)
            calls += 1
            procname = fe.read_str_va(cur_proc + self.lookups["name_offset"], 0)
            calls += 1
            fe.log("%5s %-16s" % (pid, procname))
            self.pslist[cur_proc] = (pid, procname)
            cur_list_entry = next_list_entry
            next_list_entry = fe.read_addr_va(cur_list_entry, 0)
            calls += 1
            i += 1
            fe.log("%s == %s?" % (hex(cur_list_entry), hex(list_head)))
            if cur_list_entry & 0xffffffffffff == list_head or i > maxi:
                fe.log("ending loop at %d" % (i,))
                end_pslist = time.time()
                # self.log.append(end_pslist-start_pslist)
                break
            procs += 1

        # self.calls.append(calls)
        # self.procs.append(procs)
        if pause:
            fe.resume_vm()

        fe.log("pslist in {sec:.6f} seconds".format(sec=end_pslist - start_pslist))
