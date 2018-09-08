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
        fe.log("in batching constructor")
        fe.set_name("batching")

        self.lookups = {
            "name_offset": int(fe.lookup_structure("task_struct", "comm")),
            "pid_offset": int(fe.lookup_structure("task_struct", "pid")),
            "tasks_offset": int(fe.lookup_structure("task_struct", "tasks")),
            "init_task": int(fe.lookup_symbol("init_task")),
        }

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        e = {
            "event_type": fe.TIMER,
            "time_value": 0.1,  # seconds
            "callback": self.timer_callback,
        }
        fe.event_register(e)

        self.log = []
        self.pslist = {}
        self.arr = {}
        self.calls = []
        self.msgs = []
        self.procs = []

        self.create_arr()

        fe.log("leaving batching constructor")

    def timer_callback(self, ctx):

        timeit_on = False
        cprof_bo = False
        once = True

        if timeit_on:
            self.log = []
            self.msgs = []
            self.procs = []
            fe.log("prepping batch obj")
            self.prep_batch_pslist_self()
            fe.pause_vm()
            fe.log("running timeit")
            t = timeit.Timer(lambda: self.do_batch_pslist_new()).timeit(number=100)
            fe.log("done timeit")
            fe.resume_vm()
            t = t / 100.0
            fe.log("timeit pslist in {sec:.6f} seconds".format(sec=t))
            self.summarize()

        if once:
            fe.log("prepping batch obj")
            self.prep_batch_pslist_self()
            gc.disable()
            fe.log("running pipeline")
            self.do_batch_pslist_new(pause=False)
            fe.log("done pipeline")
            gc.enable()
            self.summarize()

        if cprof_bo:
            self.log = []
            self.msgs = []
            self.procs = []
            self.prep_batch_pslist_self()
            fe.pause_vm()
            cProfile.runctx(
                "self.do_batch_pslist_new()",
                globals(),
                locals(),
                filename="/home/micah/femain.pipeline.profile",
            )
            fe.resume_vm()
            self.summarize()

        fe.log("calls: %s" % self.calls)
        fe.log("procs: %s" % self.procs)

        fe.log("calling exit")
        fe.exit()

    def summarize(self):
        for l in self.log:
            fe.log(l)
        fe.log("average:")
        fe.log(sum(self.log) / len(self.log))

    def prep_batch_pslist_self(self):
        list_head = self.lookups["init_task"] + self.lookups["tasks_offset"]
        cur = list_head
        pido = self.lookups["pid_offset"]
        tasko = self.lookups["tasks_offset"]
        nameo = self.lookups["name_offset"]

        o = fe.batch_new()
        o.add("000", "PAUSE_VM", {"status": 1})
        o.add("01", "READ_STR_VA", {"vaddr": cur + nameo - tasko, "pid": 0})
        o.add("02", "READ_32_VA", {"vaddr": cur + pido - tasko, "pid": 0})
        o.add("03", "READ_ADDR_VA", {"vaddr": cur, "pid": 0})
        for i in range(1, 120):
            o.add(
                f"{i}1",
                "READ_STR_VA",
                {"vaddr": nameo - tasko, "pid": 0},
                f"{i-1}3",
                "result",
                "vaddr",
                fe.ADD,
            )
            o.add(
                f"{i}2",
                "READ_32_VA",
                {"vaddr": pido - tasko, "pid": 0},
                f"{i-1}3",
                "result",
                "vaddr",
                fe.ADD,
            )
            o.add(
                f"{i}3",
                "READ_ADDR_VA",
                {"vaddr": 0, "pid": 0},
                f"{i-1}3",
                "result",
                "vaddr",
                fe.ADD,
            )
        o.add("001", "RESUME_VM", {"status": 1})
        o.serialize()

        self.o = o

    def do_batch_pslist_new(self, pause=False):
        if pause:
            fe.pause_vm()
        fe.log("batch_pslist_new")

        if self.o.state in [3, 4]:
            self.o.state = 2
        procs = 0
        start_pslist = time.time()

        fe.batch_send(self.o)

        stop_pslist = time.time()
        self.log.append(stop_pslist - start_pslist)
        self.procs.append(self.o.results_size())
        self.msgs.append(1)

        if pause:
            fe.resume_vm()

        fe.log("Results")
        for r in o.results():
            fe.log(r)

    def be_callback(self, ctx):
        fe.log("BE CALLBACK")
        fe.log("    passed: {ctx}".format(ctx=ctx))
