#!/usr/bin/python3 -EOO
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
"""
Facilities partition runtime.  Formats and sends IPC between apps and backends.
"""

# ./facilities.py -d --u2 123-123-123 --qt=cpu.cfs_quota_us:20000,cpu.cfs_period_us:100000,pids.max:30,memory.limit_in_bytes:300m,_furnace_max_disk:64000000 --kp /opt/furnace/f_keys --it 5561 --ip 127.0.0.1

# stdlib
import time
import argparse
import sys
import re
import os

sys.path.insert(0, os.path.abspath("/opt/furnace/shared"))

# 3p
import zmq
import zmq.auth
from zmq.auth.thread import ThreadAuthenticator
from google.protobuf.internal import api_implementation
import facilities_pb2

# internal
from constants import *
from furnace_runtime import D, DEBUG, I, INFO, N, NOTICE, W, WARN, WARNING
from furnace_runtime import E, ERR, ERROR, C, CRIT, CRITICAL
import furnace_runtime


class Facilities(furnace_runtime.FurnaceRuntime):
    """
    Main Facilities class.
    """

    def __init__(self, debug, u2, kp, qt, it, ip, fac):
        """
        Constructor, ZMQ connections to the APP partition and backend are built here.
        :param u2: The UUID for the corresponding app instance.
        :param kp: The keypair to use, in the form {'be_key': '[path]', 'app_key': '[path]'}
        :param qt: Custom quotas dict.
        :param ip: Connect to this proxy IP.
        :param it: Connect to this proxy TCP port.
        :param fac: The path to the FAC partition's ZMQ socket.
        """

        super(Facilities, self).__init__(debug=debug)
        self.debug = debug
        self.u2 = u2
        self.fac_path = fac
        self.it = it
        self.ip = ip
        self.already_shutdown = False

        self.tprint(DEBUG, "starting Facilities constructor")
        self.tprint(DEBUG, f"running as UUID={u2}")

        api = api_implementation._default_implementation_type
        if api == "python":
            raise Exception("not running in cpp accelerated mode")
        self.tprint(DEBUG, f"running in api mode: {api}")

        self.fmsg_in = facilities_pb2.FacMessage()
        self.fmsg_out = facilities_pb2.FacMessage()
        self.pmsg_in = facilities_pb2.FacMessage()
        self.pmsg_out = facilities_pb2.FacMessage()

        self.storage = {}
        self.io_id = 0
        self.disk_usage = 0
        self.max_disk_usage = int(qt["_furnace_max_disk"])
        self.tprint(DEBUG, f"MAX_DISK_USAGE {self.max_disk_usage}")

        self.context = zmq.Context(io_threads=4)
        self.poller = zmq.Poller()

        # connections with the proxy
        self.tprint(DEBUG, f"loading crypto...")
        self.auth = ThreadAuthenticator(self.context)  # crypto bootstrap
        self.auth.start()
        self.auth.configure_curve(domain="*", location=zmq.auth.CURVE_ALLOW_ANY)
        publisher_public, _ = zmq.auth.load_certificate(kp["be_pub"])
        public, secret = zmq.auth.load_certificate(kp["app_key"])

        assert self.auth.is_alive()
        self.tprint(DEBUG, f"loading crypto... DONE")

        TCP_PXY_RTR = f"tcp://{str(ip)}:{int(it)+0}"
        TCP_PXY_PUB = f"tcp://{str(ip)}:{int(it)+1}"

        # connections to the proxy
        s = f"tcp://{self.ip}:{self.it}"
        self.dealer_pxy = self.context.socket(zmq.DEALER)  # async directed
        self.didentity = f"{u2}-d"  # this must match the FE
        self.dealer_pxy.identity = self.didentity.encode()
        self.dealer_pxy.curve_publickey = public
        self.dealer_pxy.curve_secretkey = secret
        self.dealer_pxy.curve_serverkey = publisher_public
        self.tprint(INFO, f"FAC--PXY: Connecting as Dealer to {TCP_PXY_RTR}")
        self.dealer_pxy.connect(s)

        self.req_pxy = self.context.socket(zmq.REQ)  # sync
        self.ridentity = f"{u2}-r"
        self.req_pxy.identity = self.ridentity.encode()
        self.req_pxy.curve_publickey = public
        self.req_pxy.curve_secretkey = secret
        self.req_pxy.curve_serverkey = publisher_public
        self.req_pxy.connect(s)

        self.sub_pxy = self.context.socket(zmq.SUB)  # async broadcast
        self.sub_pxy.curve_publickey = public
        self.sub_pxy.curve_secretkey = secret
        self.sub_pxy.curve_serverkey = publisher_public
        self.sub_pxy.setsockopt(zmq.SUBSCRIBE, b"")
        self.tprint(INFO, f"FAC--PXY: Connecting as Publisher to {TCP_PXY_PUB}")
        self.sub_pxy.connect(TCP_PXY_PUB)

        self.poller.register(self.dealer_pxy, zmq.POLLIN)
        self.poller.register(self.sub_pxy, zmq.POLLIN)

        # connections with the frontend
        s = f"ipc://{self.fac_path}{os.sep}fac_rtr"
        self.rtr_fac = self.context.socket(zmq.ROUTER)  # fe sync/async msgs
        self.tprint(INFO, f"APP--FAC: Binding as Router to {s}")
        try:
            self.rtr_fac.bind(s)
        except zmq.error.ZMQError:
            self.tprint(ERROR, f"rtr_fac {s} is already in use!")
            sys.exit()

        s = f"ipc://{self.fac_path}{os.sep}fac_pub"
        self.pub_fac = self.context.socket(zmq.PUB)  # bcast to fe
        self.tprint(INFO, f"APP--FAC: Binding as Publisher to {s}")
        try:
            self.pub_fac.bind(s)
        except zmq.error.ZMQError:
            self.tprint(ERROR, f"pub_fac {s} is already in use!")
            sys.exit()

        self.poller.register(self.rtr_fac, zmq.POLLIN)
        self.tprint(DEBUG, "leaving Facilities constructor")

    def loop(self):
        """
        Main event loop.  Polls (with timeout) on ZMQ sockets waiting for data from the APP partition and backend.
        :returns: Nothing.
        """

        while True:
            socks = dict(self.poller.poll(TIMEOUT_FAC))

            # inbound async from BE (sync should not arrive here)
            if self.dealer_pxy in socks:
                sys.stdout.write("v")
                sync = ASYNC
                raw_msg = self.dealer_pxy.recv()
                self.msgin += 1
                raw_out = self.be2fe_process_protobuf(raw_msg, sync)

            # inbound broadcast from BE
            if self.sub_pxy in socks:
                sys.stdout.write("v")
                raw_msg = self.sub_pxy.recv()
                self.msgin += 1
                self.pmsg_in.ParseFromString(raw_msg)
                self.tprint(DEBUG, "BROADCAST FROM PXY\n%s" % (self.pmsg_in,))
                index = 0
                submsg_list = getattr(self.pmsg_in, "be_msg")
                m = submsg_list[index]

                self.pmsg_out.Clear()
                msgnum = self.pmsg_out.__getattribute__("BE_MSG")
                self.pmsg_out.type.append(msgnum)
                submsg = getattr(self.pmsg_out, "be_msg").add()
                submsg.status = m.status
                submsg.value = m.value
                raw_str = self.pmsg_out.SerializeToString()
                self.tprint(DEBUG, "to FE {len(raw_str)}B {self.pmsg_out}")
                self.pub_fac.send(raw_str)
                self.msgout += 1

            # inbound from FE
            if self.rtr_fac in socks:
                sys.stdout.write("^")
                pkt = self.rtr_fac.recv_multipart()
                self.msgin += 1
                if len(pkt) == 3:  # sync
                    sync = SYNC
                    ident, empty, raw_msg = pkt
                    raw_out = self.fe2be_process_protobuf(raw_msg, sync)
                    self.rtr_fac.send_multipart([ident.encode(), b"", raw_out.encode()])
                    self.msgout += 1
                elif len(pkt) == 2:  # async
                    sync = ASYNC
                    ident, raw_msg = pkt
                    self.fe2be_process_protobuf(raw_msg, sync)

            if not socks:
                sys.stdout.write(".")
            sys.stdout.flush()

            self.tick += 1

    def be2fe_process_protobuf(self, raw_msg, sync):
        """
        Processes messages sent by the backend.
        :returns: Nothing (modifies class's protobufs).
        """
        self.pmsg_in.ParseFromString(raw_msg)
        self.tprint(DEBUG, "FROM BE\n%s" % (self.pmsg_in,))

        for t in self.pmsg_in.type:
            msgtype = facilities_pb2.FacMessage.Type.Name(t)  # msgtype = BE_MSG
            msglist = getattr(self.pmsg_in, msgtype.lower())
            m = msglist.pop(0)  # the particular message
            if msgtype == "BE_MSG":
                self.fmsg_out.Clear()
                msgnum = self.fmsg_out.__getattribute__("BE_MSG")
                self.fmsg_out.type.append(msgnum)
                submsg = getattr(self.fmsg_out, "be_msg").add()
                submsg.status = m.status
                submsg.value = m.value
                raw_str = self.fmsg_out.SerializeToString()
                self.tprint(
                    DEBUG, "to FE {self.didentity} {len(raw_str)}B {self.fmsg_out}"
                )
                self.rtr_fac.send_multipart([self.didentity.encode(), raw_str])
                self.msgout += 1

        return

    # Split off between pass-through to BE or fac-provided IO.
    def fe2be_process_protobuf(self, raw_msg, sync):
        """
        Processes messages sent by the app.  Some messages are processed locally (IO).
        Others are forwarded to the backend.
        :returns: Raw serialized protobuf.
        """
        self.fmsg_in.ParseFromString(raw_msg)
        self.tprint(DEBUG, "FROM FE\n%s" % (self.fmsg_in,))

        self.fmsg_out.Clear()
        for t in self.fmsg_in.type:
            msgtype = facilities_pb2.FacMessage.Type.Name(t)  # msgtype = BE_MSG
            msglist = getattr(self.fmsg_in, msgtype.lower())
            m = msglist.pop(0)  # the particular message
            if msgtype == "BE_MSG":  # Pass-through to BE
                self.fe2be_process_msg(m, sync)
            elif msgtype == "GET":  # IO
                self.process_get(m)
            elif msgtype == "SET":  # IO
                self.process_set(m)

        self.tprint(DEBUG, "sending {self.fmsg_out}")
        raw_out = self.fmsg_out.SerializeToString()
        return raw_out

    def fe2be_process_msg(self, m, sync):
        """
        Processes messages sent by the app.  All these are destined to the backend.
        :returns: Nothing (modifies class's protobufs).
        """
        self.pmsg_out.Clear()
        msgnum = self.pmsg_out.__getattribute__("BE_MSG")
        self.pmsg_out.type.append(msgnum)
        submsg = getattr(self.pmsg_out, "be_msg").add()
        submsg.status = m.status
        submsg.value = m.value
        raw_str = self.pmsg_out.SerializeToString()
        self.tprint(DEBUG, "sending to BE %dB %s" % (len(raw_str), self.pmsg_out))

        if sync == SYNC:
            self.req_pxy.send(raw_str)
            self.msgout += 1
            self.pmsg_in.ParseFromString(self.req_pxy.recv())
            self.msgin += 1
            self.tprint(DEBUG, "recv from BE %dB %s" % (len(raw_str), self.pmsg_in))

            index = 0
            submsg_list = getattr(self.pmsg_in, "be_msg_ret")
            status = submsg_list[index].status
            val = submsg_list[index].value

            self.fmsg_out.Clear()
            msgnum = self.fmsg_out.__getattribute__("BE_MSG_RET")
            self.fmsg_out.type.append(msgnum)
            submsg = getattr(self.fmsg_out, "be_msg_ret").add()
            submsg.status = status
            submsg.value = val
            # raw_str = self.pmsg_out.SerializeToString()
            # return raw_str
            return

        elif sync == ASYNC:
            self.dealer_pxy.send(raw_str)
            self.msgout += 1
            return

    def process_get(self, m):
        """
        The app is asking for previously stored data.
        :param m: The protobuf message sent by the app.
        :returns: Nothing (modifies class's protobufs).
        """
        key = str(m.key)
        msgtype = "GET_RET"
        msgnum = self.fmsg_out.__getattribute__(msgtype)
        self.fmsg_out.type.append(msgnum)
        submsg = getattr(self.fmsg_out, msgtype.lower()).add()
        submsg.key = key
        if not self.get_name_ok(key):
            submsg.value = ""
            submsg.result = VMI_FAILURE
            return
        try:
            io_id = self.storage[key]
            with open(FAC_IO_DIR + str(io_id), "rb") as fh:
                submsg.value = fh.read().decode()
            submsg.result = VMI_SUCCESS
        except IndexError:
            submsg.value = ""
            submsg.result = VMI_FAILURE
        return

    def process_set(self, m):
        """
        The app has requested we store this data.
        :param m: The protobuf message sent by the app.
        :returns: Nothing (modifies class's protobufs).
        """
        key = str(m.key)
        value = str(m.value)
        msgtype = "SET_RET"
        msgnum = self.fmsg_out.__getattribute__(msgtype)
        self.fmsg_out.type.append(msgnum)
        submsg = getattr(self.fmsg_out, msgtype.lower()).add()
        submsg.key = key
        if not self.get_name_ok(key):
            self.tprint(ERROR, f"key not allowed")
            submsg.value = ""
            submsg.result = VMI_FAILURE
            return

        self.tprint(
            DEBUG,
            f"cur={self.disk_usage}, req={len(value.encode())}, max={MAX_DISK_USAGE}",
        )
        if self.disk_usage + len(value.encode()) > MAX_DISK_USAGE:
            self.tprint(ERROR, f"disk usage exceeded")
            submsg.result = VMI_FAILURE
            return

        try:
            # io_id = str(uuid.uuid4())

            self.tprint(DEBUG, f"checking for existing key {key}")
            # overwrite a key
            try:
                existing_id = self.storage[key]
                self.tprint(DEBUG, f"found!")
            except KeyError:
                self.tprint(DEBUG, f"not found")
                pass
            else:
                s = os.path.getsize(FAC_IO_DIR + str(existing_id))
                self.tprint(DEBUG, f"deleting existing file of size={s}")
                self.disk_usage -= s
                os.remove(FAC_IO_DIR + str(existing_id))
                del self.storage[key]

            io_id = self.io_id
            self.io_id += 1
            self.tprint(DEBUG, f"will assign key {io_id}")
            self.tprint(DEBUG, f"writing file of size {len(value.encode())}")
            self.disk_usage += len(value)
            self.tprint(DEBUG, f"disk usage at {self.disk_usage}")
            self.storage[key] = io_id
            with open(FAC_IO_DIR + str(io_id), "wb") as fh:
                fh.write(value.encode())
            submsg.result = VMI_SUCCESS
        except:
            submsg.result = VMI_FAILURE
        return

    def get_name_ok(self, n):
        """
        :param n: The key (must be alphanum and <=16 characters).
        :returns: True if in accordance with naming convention, False if otherwise.
        """
        return 0 < len(n) < 17 and re.match("^[\w-]+$", n)

    def check_encoding(self, val):
        """
        Fun.  Now with more Unicode.
        :param val: Is this variable a string?
        :returns: Nothing.
        :raises: TypeError if val is not a string.
        """
        if not isinstance(val, str):
            raise TypeError("Error: value is not of type str")

    def shutdown(self):
        """
        Exit cleanly.
        :returns: Nothing.
        """
        if self.already_shutdown:
            self.tprint(ERROR, "already shutdown?")
            return

        self.already_shutdown = True
        for key in self.storage:
            io_id = self.storage[key]
            self.tprint(INFO, "removing io: %s -> %s" % (key, io_id))
            os.remove(FAC_IO_DIR + str(io_id))

        self.auth.stop()

        self.poller.unregister(self.dealer_pxy)
        self.poller.unregister(self.sub_pxy)
        self.dealer_pxy.close()
        self.sub_pxy.close()

        self.poller.unregister(self.rtr_fac)
        self.rtr_fac.close()
        self.pub_fac.close()

        self.context.destroy()
        super(Facilities, self).shutdown()


def main():
    """
    Parses command line input, sets up env, then runs the facilities runtime.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-d",
        dest="debug",
        default=False,
        action="store_true",
        help="Enable debugging to console",
    )
    parser.add_argument(
        "--u2",
        dest="u2",
        required=True,
        metavar="uuid2",
        help="UUID2 for App/Fac/Pxy instances",
    )
    parser.add_argument(
        "--kp",
        dest="kp",
        required=True,
        metavar="keypair",
        help="Keypair dir (be.pub and app.key)",
    )
    parser.add_argument(
        "--qt",
        dest="qt",
        required=True,
        metavar="quotas",
        help="Quota string (k:v,k:v)",
    )
    parser.add_argument(
        "--it",
        dest="it",
        required=True,
        metavar="internal_port",
        help="Internal TCP port of proxy",
    )
    parser.add_argument(
        "--fac",
        dest="fac",
        default="/opt/furnace/furnace_sandbox/tenant_data",
        metavar="path",
        help="Path to facilities partition sockets",
    )
    parser.add_argument(
        "--ip",
        dest="ip",
        required=True,
        metavar="internal_ip",
        help="Internal IP of proxy",
    )
    args = parser.parse_args()
    print(f'{"#"*10}\nmain, starting Facilities with args: {args}\n{"#"*10}')

    if (
        not os.path.isdir(args.kp)
        or not os.path.isfile(args.kp + os.sep + "be.pub")
        or not os.path.isfile(args.kp + os.sep + "app.key")
    ):
        print(
            f"ERROR: either {args.kp} is not a directory or be.pub/app.key are missing!"
        )
        sys.exit()
    kp = {
        "be_pub": args.kp + os.sep + "be.pub",
        "app_key": args.kp + os.sep + "app.key",
    }
    print(f"startup: keys OK")

    if not os.path.isdir(args.fac):
        print(f"ERROR: {args.fac} is not a directory!")
        sys.exit()

    qt = {}
    try:
        for b in args.qt.split(","):
            k, v = b.split(":")
            qt[k] = v
    except:
        print(f"ERROR: could not parse quotas: {args.qt}")
        sys.exit()
    print(f"startup: quotas OK")

    try:
        fac = Facilities(
            debug=args.debug,
            u2=args.u2,
            kp=kp,
            qt=qt,
            it=args.it,
            ip=args.ip,
            fac=args.fac,
        )

        # signal to ptracer to strengthen the policy
        print(f"startup: signaling to ptracer to strengthen its policy")
        try:
            fh = open("ESTABLISHED", "r")
        except FileNotFoundError as e:
            print(f"startup: ESTABLISHED {e}")
        print("startup: established")
        # end signal

        fac.loop()
    except KeyboardInterrupt:
        print("exiting...")
        fac.shutdown()


if __name__ == "__main__":
    main()
