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

# import hashlib
import binascii

import zlib


class App(object):
    def __init__(self, ctx):
        fe.log("in memdump constructor")
        fe.set_name("memdump")

        fe.log("getting memsize")
        self.memsize = fe.get_memsize()
        fe.log(f"memsize: {self.memsize}")

        self.offset = 0
        self.ttl = 100
        # self.bound = fe.MAX_FIELD_SIZE - 1024
        # self.bound = 8096
        self.bound = 2 ** 22
        self.did = -1

        fe.event_register({"event_type": fe.BE, "callback": self.be_callback})
        fe.event_register(
            {"event_type": fe.TIMER, "time_value": 2.0, "callback": self.timer_callback}
        )

        self.ticks = 0
        self.one_way = False
        self.two_way = False

        self.zco = zlib.compressobj()

        fe.log("leaving memdump constructor")

    def s(self):
        return f""

    def memdump(self):
        if self.offset + self.bound > self.memsize:
            fe.log("memsize will be exceeded")
            bound = self.memsize - self.offset
            if bound == 0:
                fe.log("bound==0, all done!")
                raise IndexError("bound==0, all done!")
        else:
            bound = self.bound

        res = fe.read_pa(self.offset, bound)
        fe.log(f"read {bound}B from offset={self.offset}, got {len(res)}B")
        self.offset += bound
        return res

    def memdump_callback(self, ctx):
        fe.log(
            f"memdump_callback: coff={self.offset}, cttl={self.ttl}, memsize={self.memsize}"
        )

        if self.ttl > 0:
            self.ttl -= 1

            read_size = 0
            batch_size = 0
            chunk_arr = []
            while batch_size < self.bound:
                try:
                    mem = self.memdump()
                except IndexError as e:
                    fe.log(f"caught {e}, all done!")
                    self.ttl = 0
                    break
                else:
                    zmem = self.zco.compress(mem)
                    batch_size += len(zmem)
                    chunk_arr.append(binascii.b2a_base64(zmem).decode())
                    fe.log(f"batch {batch_size} {len(mem)} -> {len(zmem)}")

            # res = self.memdump()
            # res_utf = binascii.b2a_base64(res).decode()
            fe.log(f'sending {self.ticks}:{batch_size} B to BE, "{chunk_arr[0][0:16]}"')
            fe.notify(
                json.dumps({"cmd": "memdump", "data": chunk_arr, "ix": self.ticks})
            )  # 'hash': hashlib.sha256(res).hexdigest()}))
            # TODO: for benchmarking, we do not send a hash to the be
        else:
            fe.log("ttl < 1, clearing memdump callback timer")
            fe.event_clear(self.did)
            fe.notify(json.dumps({"cmd": "memdump_done", "data": time.time()}))
            self.did = -1

        self.ticks += 1

    def be_callback(self, ctx):
        fe.log(f"BE CALLBACK: {ctx}")
        self.two_way = True

        # msg format: {'cmd': 'foo', 'data': 'bar', 'ix': dump_order, 'hash': hash}
        # ALL ASYNC
        # FE: hi, waiting, memdump_running, memdump_done, error
        # BE: memdump_cmd: go, stop
        msg = json.loads(ctx.message)
        fe.log(f"JSON message: {msg}")

        if msg["cmd"] == "memdump_cmd" and msg["data"] == "go":
            if self.did >= 0:
                fe.log(f"error, dump already in procress (eid={self.did})")
                fe.notify(
                    json.dumps({"cmd": "error", "data": "memdump already running"})
                )
            else:
                fe.log(f"running memdump")
                fe.notify(json.dumps({"cmd": "memdump_running", "data": time.time()}))
                self.did = fe.event_register(
                    {
                        "event_type": fe.TIMER,
                        "time_value": 0.0,
                        "callback": self.memdump_callback,
                    }
                )
                fe.log(f"registered new event {self.did}")

        elif msg["cmd"] == "memdump_cmd" and msg["data"] == "stop":
            if self.did >= 0:
                fe.event_clear(self.did)
                self.did = -1
                fe.log(f"canceled dump")
            else:
                fe.log(f"error, no dump in procress (eid={self.did})")

        elif msg["cmd"] == "memdump_cmd" and msg["data"] == "exit":
            fe.log("was commanded to exit...")
            fe.exit()

        fe.log("BE CALLBACK done")

    def shutdown(self):  # destructor
        fe.log("SHUTDOWN")

    def timer_callback(self, ctx):
        if not self.one_way:
            fe.log("hi")
            fe.notify(json.dumps({"cmd": "hi", "data": time.time()}))
            self.one_way = True
        elif not self.two_way:
            fe.log("waiting")
            fe.notify(json.dumps({"cmd": "waiting", "data": time.time()}))

        self.ticks += 1
