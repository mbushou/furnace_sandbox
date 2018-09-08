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

        self.pslist = {}
        self.log = []
        self.calls = []
        self.msgs = []
        self.procs = []

        fe.log("leaving batching constructor")

    def timer_callback(self, ctx):

        timeit_on = False
        cprof = False
        once = True

        if cprof:
            self.do_batch_pslist(pause=True)  # zmq warmup
            cProfile.runctx(
                "self.do_batch_pslist()",
                globals(),
                locals(),
                filename="/home/micah/femain.batch.profile",
            )

        if timeit_on:
            fe.log("warming up")
            self.do_batch_pslist(pause=True)  # zmq warmup
            fe.pause_vm()
            fe.log("running timeit")
            t = timeit.Timer(lambda: self.do_batch_pslist()).timeit(number=100)
            fe.log("done timeit")
            fe.resume_vm()
            t = t / 100.0
            fe.log("timeit pslist in {sec:.6f} seconds".format(sec=t))

        if once:
            self.pslist = {}
            gc.disable()
            self.do_batch_pslist(pause=True)
            gc.enable()
            # fe.log(self.pslist)

        for l in self.log:
            fe.log(l)
        fe.log("calls: %s" % self.calls)
        fe.log("procs: %s" % self.procs)
        fe.log("msgs: %s" % self.msgs)
        fe.log("leaving batching constructor")
        fe.exit()

    def do_batch_pslist(self, pause=False):
        if pause:
            fe.pause_vm()

        calls = 0
        procs = 0
        msgs = 0
        i = 0
        maxi = 100

        pido = self.lookups["pid_offset"]
        tasko = self.lookups["tasks_offset"]
        nameo = self.lookups["name_offset"]
        list_head = self.lookups["init_task"] + self.lookups["tasks_offset"]
        cur = list_head
        o = fe.batch_new()

        start_pslist = time.time()

        while True:

            o.reset()
            o.add(
                "1", "READ_STR_VA", {"vaddr": cur + nameo - tasko, "pid": 0}
            )  # current name
            o.add(
                "2", "READ_32_VA", {"vaddr": cur + pido - tasko, "pid": 0}
            )  # current pid
            o.add("3", "READ_ADDR_VA", {"vaddr": cur, "pid": 0})  # pointer to next ts
            fe.batch_send(o)
            calls += 3
            msgs += 1
            procs += 1

            for r in o.results():
                if r["name"] == "3":
                    cur = r["result"]
                # do something with result

            i += 1
            if cur & 0xffffffffffff == list_head or i > maxi:
                fe.log(f"ending loop at {i}")
                end_pslist = time.time()
                self.log.append(end_pslist - start_pslist)
                break

        self.calls.append(calls)
        self.procs.append(procs)
        self.msgs.append(msgs)

        if pause:
            fe.resume_vm()

    def be_callback(self, ctx):
        fe.log("BE CALLBACK")
        fe.log("    passed: {ctx}".format(ctx=ctx))
