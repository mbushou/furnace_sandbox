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
import subprocess
import time
import json


class App(object):
    def __init__(self, ctx):
        fe.log("in rekall constructor")
        fe.set_name("rekall")

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        e = {
            "event_type": fe.TIMER,
            "time_value": 2.0,  # seconds
            "callback": self.timer_callback,
        }
        fe.event_register(e)
        self.ticks = 0
        self.one_way = False
        self.two_way = False

        fe.log("leaving rekall constructor")

    def be_callback(self, ctx):
        fe.log(f"BE CALLBACK: {ctx}")
        self.two_way = True

        # msg format: {'cmd': 'foo', 'data': 'bar'}
        # ALL ASYNC
        # FE: hi, waiting, rekall_running, rekall_done
        # BE: rekall_cmd
        msg = json.loads(ctx.message)
        fe.log(f"JSON message: {msg}")

        if msg["cmd"] == "rekall_cmd":
            # cmd = '/opt/rekall/bin/rekal -p /opt/kernels/rekall/win7ie11.rekall.json -vf /home/furnace pslist'
            cmd = msg["data"]
            fe.log(f"running {cmd}")

            fe.notify(json.dumps({"cmd": "rekall_running", "data": time.time()}))
            stime = time.time()
            output = subprocess.check_output(cmd.split())
            ttime = time.time() - stime

            fe.log(f"REKALL OUTPUT: {output}")
            fe.log(f"got {len(output)}B output, sending to BE")
            fe.log(f"time elapsed {ttime}")
            fe.notify(json.dumps({"cmd": "rekall_done", "data": output.decode()}))
            # fe.notify(json.dumps({'cmd': 'waiting', 'data': time.time()}))

        fe.log("BE CALLBACK done")

    def timer_callback(self, ctx):
        if not self.one_way:
            fe.log("hi")
            fe.notify(json.dumps({"cmd": "hi", "data": time.time()}))
            self.one_way = True
        elif not self.two_way:
            fe.log("waiting")
            fe.notify(json.dumps({"cmd": "waiting", "data": time.time()}))

        self.ticks += 1
