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
        fe.set_name("modlist")
        self.pslist = {}

        self.lookups = {
            "name_offset": fe.lookup_structure("task_struct", "comm"),
            "pid_offset": fe.lookup_structure("task_struct", "pid"),
            "tasks_offset": fe.lookup_structure("task_struct", "tasks"),
            "modules": fe.lookup_symbol("modules"),
        }

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        self.do_modlist()

        fe.exit()

    def shutdown(self):  # destructor
        pass

    def be_callback(self, ctx):
        fe.log("{ctr:<3} BE CALLBACK".format(ctr=self.cr3_ctr))
        fe.log("    passed: {ctx}".format(ctx=ctx))

    def do_modlist(self):
        fe.pause_vm()
        start_pslist = time.time()

        list_head = self.lookups["modules"]
        next_module = self.lookups["modules"]

        i = 0
        maxi = 100
        fe.log("starting MODLIST")
        while True:

            tmp_next = fe.read_addr_va(next_module, 0)

            if tmp_next & 0xffffffffffff == list_head or i > maxi:
                fe.log("ending loop at %d" % (i,))
                end_pslist = time.time()
                break

            modname = fe.read_str_va(next_module + 16, 0)
            fe.log("%s" % (modname,))

            next_module = tmp_next

            i += 1

        stop_pslist = time.time()
        fe.resume_vm()
        fe.log("modlist in {sec:.6f} seconds".format(sec=stop_pslist - start_pslist))
