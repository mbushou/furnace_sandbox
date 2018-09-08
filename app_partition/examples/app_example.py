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
        fe.log("in Example constructor")
        fe.set_name("example")
        self.cr3_ctr = 0
        self.cr3_event = None
        self.pslist = {}

        self.lookups = {
            "name_offset": fe.lookup_structure("task_struct", "comm"),
            "pid_offset": fe.lookup_structure("task_struct", "pid"),
            "tasks_offset": fe.lookup_structure("task_struct", "tasks"),
            "init_task": fe.lookup_symbol("init_task"),
        }

        fe.events_start()
        e = {
            "event_type": fe.REG,
            "reg_type": fe.CR3,
            "sync": fe.ASYNC,
            "callback": self.cr3_callback,
        }
        self.cr3_event = fe.event_register(e)
        fe.log("registered CR3 event: {eid}".format(eid=self.cr3_event))

        e = {
            "event_type": fe.TIMER,
            "time_value": 10.0,  # seconds
            "callback": self.timer_callback,
        }
        fe.event_register(e)

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        fe.log("leaving Example constructor")

    def shutdown(self):  # destructor
        if not self.cr3_event == None:
            fe.event_clear(self.cr3_event)
            self.cr3_event = None

    def cr3_callback(self, ctx):
        fe.log("{ctr:<3} CR3 CALLBACK".format(ctr=self.cr3_ctr))
        # fe.log('    passed: {ctx}'.format(ctx=ctx))
        self.cr3_ctr += 1

    def be_callback(self, ctx):
        fe.log("{ctr:<3} BE CALLBACK".format(ctr=self.cr3_ctr))
        fe.log("    passed: {ctx}".format(ctx=ctx))

    def timer_callback(self, ctx):
        fe.log("{ctr:<3} TIMER CALLBACK".format(ctr=self.cr3_ctr))
        fe.log("    passed: {ctx}".format(ctx=ctx))

        self.do_pslist()

        if self.cr3_ctr and not (self.cr3_ctr % 2) and not self.cr3_event == None:
            fe.log("clearing CR3 event")
            fe.event_clear(self.cr3_event)
            self.cr3_event = None

    def __str__(self):
        buf = "runtime context {\n"
        buf += " .cr3_ctr: %d\n" % (self.cr3_ctr,)
        buf += " .cr3_event: %s\n" % (self.cr3_event,)
        buf += "}"
        return buf

    def do_pslist(self):
        fe.pause_vm()
        start_pslist = time.time()

        list_head = self.lookups["init_task"] + self.lookups["tasks_offset"]
        cur_list_entry = list_head
        next_list_entry = fe.read_addr_va(list_head, 0)

        i = 0
        maxi = 100
        fe.log("starting PSLIST")
        while True:
            cur_proc = cur_list_entry - self.lookups["tasks_offset"]

            pid = fe.read_32_va(cur_proc + self.lookups["pid_offset"], 0)
            procname = fe.read_str_va(cur_proc + self.lookups["name_offset"], 0)
            fe.log("%5s %-16s" % (pid, procname))
            cur_list_entry = next_list_entry
            next_list_entry = fe.read_addr_va(cur_list_entry, 0)
            i += 1
            # fe.log('%s == %s?' % (hex(cur_list_entry), hex(list_head)))
            if cur_list_entry & 0xffffffffffff == list_head or i > maxi:
                # fe.log('ending loop at %d' % (i,))
                end_pslist = time.time()
                break

        stop_pslist = time.time()
        fe.resume_vm()
        fe.log("pslist in {sec:.2f} seconds".format(sec=stop_pslist - start_pslist))
