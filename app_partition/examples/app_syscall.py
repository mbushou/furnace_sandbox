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
import random


class App(object):
    def __init__(self, ctx):
        fe.log("in syscalls constructor")
        fe.set_name("syscalls")

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        e = {
            "event_type": fe.TIMER,
            "time_value": 2.0,  # seconds
            "callback": self.timer_callback,
        }
        fe.event_register(e)
        self.ticks = 0
        self.calls = 0

        fe.log("leaving syscalls constructor")

    def be_callback(self, ctx):
        self.calls += 1
        fe.log(f"{self.calls} BE CALLBACK")
        fe.log(ctx)

    def timer_callback(self, ctx):
        fe.log(f"TIMER CALLBACK {self.ticks}")

        if self.ticks == 0:
            fe.log("dk_start_syscall")
            self.do_dk_start_syscall(self.be_callback, fe.ASYNC)

        if self.ticks == 10:
            fe.log("dk_stop_syscall")
            self.do_dk_stop_syscall()
            fe.exit()

        self.ticks += 1

    def do_dk_start_syscall(self, callback, sync):
        self.syscall_eid = fe.start_syscall(callback, sync)
        fe.log(f"got eid {self.syscall_eid}")

    def do_dk_stop_syscall(self):
        fe.log(fe.stop_syscall(self.syscall_eid))
