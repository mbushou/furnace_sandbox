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
        fe.log("in pslist_dk constructor")
        fe.set_name("pslist_dk")

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)
        self.pslist = {}
        self.log = []
        self.calls = []
        self.msgs = []
        self.procs = []

        timeit_on = False
        cprof = False

        if cprof:
            self.do_dk_pslist(pause=True)  # zmq warmup
            cProfile.runctx(
                "self.do_dk_pslist()",
                globals(),
                locals(),
                filename="/home/micah/femain.dk.profile",
            )

        if timeit_on:
            self.do_dk_pslist(pause=True)  # zmq warmup
            fe.pause_vm()
            t = timeit.Timer(lambda: self.do_dk_pslist()).timeit(number=100)
            t = t / 100.0
            fe.resume_vm()
            fe.log("timeit pslist in {sec:.6f} seconds".format(sec=t))
        else:
            self.pslist = {}
            gc.disable()
            self.do_dk_pslist(pause=True)
            gc.enable()
            fe.log(self.pslist)

        for l in self.log:
            fe.log(l)
        fe.log("calls: %s" % self.calls)
        fe.log("procs: %s" % self.procs)
        fe.log("msgs: %s" % self.msgs)
        fe.log("leaving pslist_dk constructor")
        fe.exit()

    def do_dk_pslist(self, pause=False):
        if pause:
            fe.pause_vm()

        start_pslist = time.time()
        self.pslist = fe.process_list()

        self.procs.append(len(self.pslist))
        self.msgs.append(1)

        stop_pslist = time.time()
        self.log.append(stop_pslist - start_pslist)

        if pause:
            fe.resume_vm()

    def be_callback(self, ctx):
        fe.log("{ctr:<3} BE CALLBACK".format(ctr=self.cr3_ctr))
        fe.log("    passed: {ctx}".format(ctx=ctx))
