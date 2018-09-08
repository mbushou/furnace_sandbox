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
Main Furnace app library.  Exposes API to tenant code.  Formats and sends IPC.
"""

import sys
import os

import collections
import time
from pprint import pformat as pf
import re
import string

# 3p
import zmq
from google.protobuf.internal import api_implementation
import vmi_pb2
import facilities_pb2

# internal
from constants import *
from furnace_runtime import D, DEBUG, I, INFO, N, NOTICE, W, WARN, WARNING
from furnace_runtime import E, ERR, ERROR, C, CRIT, CRITICAL
import furnace_runtime
import batch_obj


class FE(furnace_runtime.FurnaceRuntime):
    """
    Main Furnace (frontend) app class.  Actions all API calls.
    """

    def __init__(self, u2, fac, vmi, debug=False):
        """
        Constructor, ZMQ connections to the VMI and FAC partitions are built here.
        :param u2: The UUID for this app instance.
        :param fac: The path to the FAC partition's ZMQ socket.
        :param vmi: The path to the VMI partition's ZMQ socket.
        """

        super(FE, self).__init__(debug=debug)

        self.u2 = u2
        self.fac_path = fac
        self.vmi_path = vmi
        self.start_time = time.time()
        self.already_shutdown = False

        self.ident_r = self.u2 + "-r"
        self.ident_d = self.u2 + "-d"
        self.tprint(DEBUG, "{ident} starting up!".format(ident=self.ident_r))
        self.tprint(DEBUG, "{ident} AKA".format(ident=self.ident_d))

        self.eventlist = {}
        self.timerlist = {}
        self.next_tid = 0
        self.be_info = None
        self.app = None
        self.EventContext = collections.namedtuple(
            "EventContext",
            "type identifier cr3 rip rbp rsp rdi rsi rdx rax vcpu uid procname pid ppid",
        )
        self.MessageContext = collections.namedtuple("MessageContent", "type message")

        api = api_implementation._default_implementation_type
        if api == "python":
            raise Exception("not running in cpp accelerated mode")
        self.tprint(DEBUG, f"running in api mode: {api}")

        self.vmsg = vmi_pb2.VMIMessage()
        self.fmsg = facilities_pb2.FacMessage()

        self.context = zmq.Context(io_threads=4)
        self.poller = zmq.Poller()

        # connections to VMI
        s = "ipc://" + self.vmi_path + os.sep + "vmi_rtr"
        self.tprint(INFO, f"connecting REQ to VMI partition at {s}")
        self.req_vmi = self.context.socket(zmq.REQ)  # sync to/from VMI
        self.req_vmi.identity = self.ident_r.encode("ascii")
        self.req_vmi.connect(s)

        self.tprint(INFO, f"connecting DEALER to VMI partition at {s}")
        self.dealer_vmi = self.context.socket(zmq.DEALER)  # async to/from VMI
        self.dealer_vmi.identity = self.ident_d.encode("ascii")
        self.dealer_vmi.connect(s)

        self.poller.register(self.dealer_vmi, zmq.POLLIN)

        # connections to FAC
        s = "ipc://" + self.fac_path + os.sep + "fac_rtr"
        self.tprint(INFO, f"connecting REQ to facilities partition at {s}")
        self.req_fac = self.context.socket(zmq.REQ)  # sync to/from FAC
        self.req_fac.identity = self.ident_r.encode("ascii")
        self.req_fac.connect(s)

        self.tprint(INFO, f"connecting DEALER to facilities partition at {s}")
        self.dealer_fac = self.context.socket(zmq.DEALER)  # async to/from FAC
        self.dealer_fac.identity = self.ident_d.encode("ascii")
        self.dealer_fac.connect(s)

        s = "ipc://" + self.fac_path + os.sep + "fac_pub"
        self.tprint(INFO, f"connecting SUB to faciltities partition at to {s}")
        self.sub_fac = self.context.socket(zmq.SUB)  # bcast from be
        self.sub_fac.setsockopt(zmq.SUBSCRIBE, b"")
        self.sub_fac.connect(s)

        self.poller.register(self.dealer_fac, zmq.POLLIN)
        self.poller.register(self.sub_fac, zmq.POLLIN)

    def module_register(self):
        """
        Internal use only.
        Glues together namespaces.
        :returns: Nothing.
        """
        globals()["fei"] = self
        r = [
            "translate_kv2p",
            "translate_uv2p",
            "translate_ksym2v",
            "read_pa",
            "read_va",
            "read_ksym",
            "read_8_pa",
            "read_16_pa",
            "read_32_pa",
            "read_64_pa",
            "read_addr_pa",
            "read_str_pa",
            "read_8_va",
            "read_16_va",
            "read_32_va",
            "read_64_va",
            "read_addr_va",
            "read_str_va",
            "read_8_ksym",
            "read_16_ksym",
            "read_32_ksym",
            "read_64_ksym",
            "read_addr_ksym",
            "read_str_ksym",
            "write_pa",
            "write_va",
            "write_ksym",
            "write_8_pa",
            "write_16_pa",
            "write_32_pa",
            "write_64_pa",
            "write_8_va",
            "write_16_va",
            "write_32_va",
            "write_64_va",
            "write_8_ksym",
            "write_16_ksym",
            "write_32_ksym",
            "write_64_ksym",
            "get_name",
            "get_vmid",
            "get_access_mode",
            "get_winver",
            "get_winver_str",
            "get_vcpureg",
            "get_memsize",
            "get_offset",
            "get_ostype",
            "get_page_mode",
            "request",
            "notify",
            "io_set",
            "io_get",
            "lookup_symbol",
            "lookup_structure",
            "events_start",
            "events_stop",
            "pause_vm",
            "resume_vm",
            "event_register",
            "event_clear",
            "log",
            "set_name",
            "exit",
            "batch",
            "process_list",
            "start_syscall",
            "stop_syscall",
        ]
        for k in r:
            globals()[k] = getattr(self, k)
        globals()["ADD"] = F_ADD
        globals()["SUB"] = F_SUB
        globals()["OVERWRITE"] = F_OVERWRITE

    def post_app_init(self, app):
        """
        Internal use only.
        Called by femain after the tenant app's constructor runs.  Checks to ensure the constructor registered exactly one BE callback.
        :param app: The tenant's app instance.
        :returns: Nothing.
        :raises: Exception when caller attempts to register 2 or more BE callbacks.
        """
        self.app = app
        be_ok = False
        for tk in self.timerlist:
            tv = self.timerlist[tk]
            if tv["event_type"] == BE:
                if be_ok:
                    errmsg = "Tenant app cannot register more than one BE callback!"
                    self.tprint(ERROR, errmsg)
                    raise Exception(errmsg)
                be_ok = True
        # FIXED: backends are optional, after all!
        if not be_ok:
            self.tprint("warn", "Tenant app did not register a BE callback!")
            # raise Exception('Tenant app failed to register BE event')
        self.tprint(DEBUG, f"post_app_init EVENTLIST:\n{pf(self.eventlist)}")
        self.tprint(DEBUG, f"post_app_init TIMERLIST:\n{pf(self.timerlist)}")

    def loop(self):
        """
        Internal use only.
        Main event loop.  Polls (with timeout) on ZMQ sockets waiting for data from the VMI or FAC partitions.  Calls tenant-registered callbacks.
        :returns: Nothing.
        """
        cleanup_eids = list()
        cleanup_tids = list()

        while True:
            socks = dict(self.poller.poll(TIMEOUT_VMI))

            # ASYNC message from VMI
            if self.dealer_vmi in socks:
                raw_msg = self.dealer_vmi.recv()
                self.msgin += 1
                self.vmsg.ParseFromString(raw_msg)
                self.tprint(DEBUG, f"ASYNC FROM VMI\n{self.vmsg}")
                self.tprint(DEBUG, f"Guest paused?: {self.vmsg.event.guest_paused}")
                guest_paused = bool(self.vmsg.event.guest_paused)
                eid = int(self.vmsg.event.identifier)

                try:
                    ev = self.eventlist[eid]
                except KeyError:
                    self.tprint(ERROR, f"eid {eid} not present in eventlist")
                else:
                    ctx = self.EventContext(
                        type="event",
                        identifier=self.vmsg.event.identifier,
                        cr3=self.vmsg.event.cr3,
                        rip=self.vmsg.event.rip,
                        rbp=self.vmsg.event.rbp,
                        rsp=self.vmsg.event.rsp,
                        rdi=self.vmsg.event.rdi,
                        rsi=self.vmsg.event.rsi,
                        rdx=self.vmsg.event.rdx,
                        rax=self.vmsg.event.rax,
                        vcpu=self.vmsg.event.vcpu,
                        uid=self.vmsg.event.uid,
                        procname=self.vmsg.event.procname,
                        pid=self.vmsg.event.pid,
                        ppid=self.vmsg.event.ppid,
                    )

                    ev["callback"](ctx)
                finally:
                    if guest_paused:
                        self.resume_vm()

            # Broadcast ASYNC message from Facilities
            if self.sub_fac in socks:
                raw_msg = self.sub_fac.recv()
                self.msgin += 1
                self.fmsg.ParseFromString(raw_msg)
                self.tprint(DEBUG, f"BROADCAST FROM BE\n{self.fmsg}")

                index = 0
                submsg_list = getattr(self.fmsg, "be_msg")
                val = submsg_list[index].value

                ctx = self.MessageContext(type="broadcast", message=val)
                if self.be_info:
                    self.be_info["callback"](ctx)
                else:
                    self.tprint("warn", "BCAST ASYNC: but app lacks a CB!")

            # Directed ASYNC message from Facilities
            if self.dealer_fac in socks:  # should only ever recv unsolicited
                raw_msg = self.dealer_fac.recv()
                self.msgin += 1
                self.fmsg.ParseFromString(raw_msg)
                self.tprint(DEBUG, f"ASYNC FROM BE\n{self.fmsg}")

                index = 0
                submsg_list = getattr(self.fmsg, "be_msg")
                val = submsg_list[index].value

                ctx = self.MessageContext(type="notify", message=val)
                if self.be_info:
                    self.be_info["callback"](ctx)
                else:
                    self.tprint("warn", "DIRECTED ASYNC: but app lacks a CB!")

            curtime = time.time()
            for ek in self.eventlist:
                ev = self.eventlist[ek]

                if ev["status"] != ACTIVE:
                    cleanup_eids.append(ek)
                    continue

            for tk in self.timerlist:
                tv = self.timerlist[tk]

                if tv["status"] != ACTIVE:
                    cleanup_tids.append(tk)
                    continue

                if (
                    tv["event_type"] == TIMER
                    and tv["last_called"] + tv["time_value"] < curtime
                ):
                    tv["last_called"] = curtime
                    d = "timer triggered"
                    tv["callback"](d)

            while len(cleanup_eids):
                del self.eventlist[cleanup_eids.pop()]
            while len(cleanup_tids):
                del self.timerlist[cleanup_tids.pop()]

            self.tick += 1

    # ---------------------------------------------

    def vmmsg_helper(self, msgtype):
        """
        Internal use only.
        Helper function to add another repeated field to a VMI protobuf.
        :param msgtype: string matching the name of a field (see .proto for valid field names)
        :returns: The submsg, ready to be populated.
        """
        self.vmsg.Clear()
        msgnum = self.vmsg.__getattribute__(msgtype)
        self.vmsg.type.append(msgnum)
        submsg = getattr(self.vmsg, msgtype.lower()).add()
        return submsg

    def vmsg_helper(self, msgtype):
        """
        Internal use only.
        Helper function to add a single field to a VMI protobuf.
        :param msgtype: string matching the name of a field (see .proto for valid field names)
        :returns: The submsg, ready to be populated.
        """
        self.vmsg.Clear()
        msgnum = self.vmsg.__getattribute__(msgtype)
        self.vmsg.type.append(msgnum)
        submsg = getattr(self.vmsg, msgtype.lower())
        return submsg

    def send_default_reply(self):
        """
        Internal use only.
        Helper function to send a prepared protobuf (self.msg_out), then block until a response is returned.
        :returns: Nothing.
        """
        self.req_vmi.send(self.vmsg.SerializeToString())
        self.msgout += 1
        # self.vmsg.Clear()
        self.vmsg.ParseFromString(self.req_vmi.recv())
        self.msgin += 1
        return

    def mget_status_result(self, msgtype, index):
        """
        Internal use only.
        Helper function to retrieve the result and status of an inbound protobuf.
        :param msgtype: String matching the name of a field (see .proto for valid field names).
        :param index: Which index (for repeated fields)?
        :returns: Tuple of (status, result).
        """
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        status = int(submsg_list[index].status)
        result = submsg_list[index].result
        return status, result

    def mget_status(self, msgtype, index):
        """
        Internal use only.
        Helper function to retrieve the status of an inbound protobuf.
        :param msgtype: String matching the name of a field (see .proto for valid field names).
        :param index: Which index (for repeated fields)?
        :returns: status
        """
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        status = int(submsg_list[index].status)
        return status

    def get_status_result(self, msgtype):
        """
        Internal use only.
        Helper function to retrieve the result and status of an inbound protobuf.
        :param msgtype: String matching the name of a field (see .proto for valid field names).
        :returns: Tuple of (status, result).
        """
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        status = int(submsg_list.status)
        result = submsg_list.result
        return status, result

    def get_status(self, msgtype):
        """
        Internal use only.
        Helper function to retrieve the status of an inbound protobuf.
        :param msgtype: String matching the name of a field (see .proto for valid field names).
        :param index: Which index (for repeated fields)?
        :returns: status
        """
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        status = int(submsg_list.status)
        return status

    # ---------------------------------------------

    def batch_new(self):
        """
        Supported API call.
        Create a new (empty) batch object.
        :returns: A batch object.
        """
        return batch_obj.batch_obj()

    def batch_send(self, bo):
        """
        Supported API call.
        Send a batch object to the VMI partition.
        :returns: Nothing.  Changes to the bo are updated by reference.
        """
        if bo.state != batch_obj.SERIALIZED:
            if bo.state != batch_obj.FILLING:
                raise Exception("batch_object state is invalid")
            bo.serialize()

        self.req_vmi.send(bo.raw_out)
        self.msgout += 1

        bo.raw_in = self.req_vmi.recv()
        self.msgin += 1
        bo.state = batch_obj.RESULTS_RAW

        return

    # ---------------------------------------------

    def prep_batch(self, batcharr):
        """
        Internal use only.
        Builds protobuf from batch array.
        :param batcharr: A two level batch array dict.  See app_batching for example format.
        :returns: Nothing.

        Dev notes:
        -- link fields are 0 indexed!
        -- link fields, 0th: index of previous message
           1st: index of field inside previous message
           2nd: index of field inside current message
           3rd: operator (add, subtract, overwrite)
        """
        self.vmsg.Clear()
        for bitem in batcharr:
            msgtype = bitem["type"]  # string eg READ_8_VA
            label = bitem["label"]
            msgnum = self.vmsg.__getattribute__(msgtype)
            self.vmsg.type.append(msgnum)
            submsg = getattr(
                self.vmsg, msgtype.lower()
            ).add()  # get the submsg, eg read_8_va msg
            if "link" in bitem.keys():
                prev_msg_ix, prev_item_ix, cur_item_ix, operator = bitem["link"]
                if operator not in ACCEPT_OPR.keys():
                    raise Exception("operator not in %s" % (ACCEPT_OPR))
                # self.tprint('debug', 'processing link %s' % (bitem,))
                linkmsg = getattr(submsg, "link")  # get the linkmsg
                linkmsg.prev_msg_ix = prev_msg_ix
                linkmsg.prev_item_ix = prev_item_ix
                linkmsg.cur_item_ix = cur_item_ix
                linkmsg.operator = operator
            else:
                self.tprint(DEBUG, "no link found")
            for varname in bitem["args"]:  # eg vaddr
                varval = bitem["args"][varname]  # eg 0x1234567
                submsg.__setattr__(varname, varval)  # store 0x1234 in vaddr
        return

    def process_batch_results(self, batcharr):
        """
        Internal use only.
        Builds protobuf from batch array.
        :param batcharr: A two level batch array dict.  See app_batching for example format.
        :returns: A two-level result dict.
        res['label'] = {'field_foo': [result], 'field_bar': [result]}
        :raises Exception: General protobuf message error.
        :raises Exception: Specific protobuf message error (error field filled).

        Dev notes:
        0. make sure if X messages go out, X messages come back, do error checking
        1. for each returned type, in order:
        2. do typecount to find ix, get its name, find its corresponding message via reflection, get index using typecount
        3. extract its values into resdict
        4. name this result according to its label
        """
        if len(self.vmsg.type) != len(self.vmsg.type):
            # print('something is wrong, out: %d, in: %d' % (len(self.vmsg.type), len(msg.type)))
            raise Exception(
                "something is wrong, out: %d, in: %d"
                % (len(self.vmsg.type), len(msg.type))
            )
        if self.vmsg.error:
            # print('got error: num:%d, loc:%d' % (self.vmsg.error_num, self.vmsg.error_loc))
            raise Exception(
                "got error: num:%d, loc:%d" % (self.vmsg.error_num, self.vmsg.error_loc)
            )
        else:
            in_array = batcharr
            i = 0
            typecount = {}
            resdict = {}
            for t in self.vmsg.type:
                # get info about batcharr
                inputline = in_array[i]  # the corresponding input line
                try:
                    prev_msg_ix, prev_item_ix, cur_item_ix, operator = inputline["link"]
                    haslink = True
                except KeyError:
                    haslink = False
                try:
                    label = inputline["label"]
                except KeyError:
                    print("%s does not have label" % (inputline,))

                # get index
                ix = typecount.get(t, 0)
                typecount[t] = ix + 1
                fname = vmi_pb2.VMIMessage.DESCRIPTOR.fields_by_number[t].name
                submsg_list = getattr(self.vmsg, fname.lower())
                submsg = submsg_list[ix]

                # start to rebuild message
                res = {}
                md = getattr(vmi_pb2, string.capwords(fname))
                for j in md.DESCRIPTOR.fields:
                    # self.tprint('debug', 'getting %s' % (j.name))
                    res[j.name] = getattr(submsg, j.name)

                resdict[label] = res
                i += 1

            self.tprint(DEBUG, typecount)
            # self.tprint('debug', pf(resdict.keys()))
            return resdict

    def batch(self, batcharr):
        """
        Supported API call.
        Process a batch array.
        :param batcharr: A two level batch array dict.  See app_batching for example format.
        :returns: A two-level result dict.
        res['label'] = {'field_foo': [result], 'field_bar': [result]}
        """
        self.prep_batch(batcharr)
        # self.tprint('debug', pf(self.vmsg))
        # a = time.time()
        self.send_default_reply()
        # self.tprint('info', 'vmi time: %s' % (time.time() - a))
        return self.process_batch_results(batcharr)

    def check_encoding(self, val):
        """
        Internal use only.
        Fun.  Now with more Unicode.
        :param val: Is this variable a string?
        :returns: Nothing.
        :raises: TypeError if val is not a string.
        """
        if not isinstance(val, str):
            raise TypeError("Error: value is not of type str")

    def notify(self, msg):
        """
        Supported API call.
        Send an async message to the backend.  Immediately returns regardless of delivery.
        :param msg: String to send.
        :returns: Nothing.
        """
        self.check_encoding(msg)
        # vmmsg_helper-ish
        msgtype = "BE_MSG"
        self.fmsg.Clear()
        msgnum = self.fmsg.__getattribute__(msgtype)
        self.fmsg.type.append(msgnum)
        submsg = getattr(self.fmsg, msgtype.lower()).add()
        # actual msg loading
        submsg.status = VMI_SUCCESS
        submsg.value = msg.encode()
        # send_default_reply-ish
        self.tprint(DEBUG, f"sending {self.fmsg}")
        self.dealer_fac.send(self.fmsg.SerializeToString())
        self.msgout += 1
        return

    def request(self, msg):
        """
        Supported API call.
        Send an sync message to the backend.  Blocks until response is received.
        :param msg: String to send.
        :returns: Nothing.
        :raises Exception: If error code was returned.
        """
        self.check_encoding(msg)
        # vmmsg_helper-ish
        msgtype = "BE_MSG"
        self.fmsg.Clear()
        msgnum = self.fmsg.__getattribute__(msgtype)
        self.fmsg.type.append(msgnum)
        submsg = getattr(self.fmsg, msgtype.lower()).add()
        # actual msg loading
        submsg.status = VMI_SUCCESS
        submsg.value = str(msg).encode()
        # send_default_reply-ish
        raw_str = self.fmsg.SerializeToString()
        self.tprint(DEBUG, f"sending {len(raw_str)}B {self.fmsg}")
        self.req_fac.send(raw_str)
        self.msgout += 1
        self.fmsg.ParseFromString(self.req_fac.recv())
        self.msgin += 1
        # mget_status-ish
        self.tprint(DEBUG, f"got {self.fmsg}")
        index = 0
        submsg_list = getattr(self.fmsg, msgtype.lower() + "_ret")
        status = int(submsg_list[index].status)
        if status == VMI_FAILURE:
            raise Exception("response received, but failure occured")
        return submsg_list[index].value

    def io_set(self, key, val):
        """
        Supported API call.
        Sends data to the FAC partition to be stored to disk.
        :param key: Label for this data.  Used for accessing the data later.
        :param val: The data.
        :returns: The key label on success.
        :raises Exception: If the FAC partition failed to store the data.
        """
        self.check_encoding(key)
        self.check_encoding(val)
        # vmmsg_helper-ish
        msgtype = "SET"
        self.fmsg.Clear()
        msgnum = self.fmsg.__getattribute__(msgtype)
        self.fmsg.type.append(msgnum)
        submsg = getattr(self.fmsg, msgtype.lower()).add()
        # actual msg loading
        submsg.key = key.encode()
        submsg.value = val.encode()
        # send_default_reply-ish
        raw_str = self.fmsg.SerializeToString()
        self.tprint(DEBUG, f"sending {len(raw_str)}B {self.fmsg}")
        self.req_fac.send(raw_str)
        self.msgout += 1
        self.fmsg.ParseFromString(self.req_fac.recv())
        self.msgin += 1
        # mget_status-ish
        self.tprint(DEBUG, f"got {self.fmsg}")
        index = 0
        submsg_list = getattr(self.fmsg, msgtype.lower() + "_ret")
        result = int(submsg_list[index].result)
        key_r = submsg_list[index].key
        # if result == VMI_FAILURE:
        #    raise Exception('response received, but failure occured')
        if not key == key_r:
            raise Exception("response received, but keys did not match")
        return key_r

    def io_get(self, key):
        """
        Supported API call.
        Gets previously stored data from the FAC partition.
        :param key: Label for previously stored data.
        :returns: The data.
        :raises Exception: If the FAC partition failed to retrieve the data.
        """
        self.check_encoding(key)
        # vmmsg_helper-ish
        msgtype = "GET"
        self.fmsg.Clear()
        msgnum = self.fmsg.__getattribute__(msgtype)
        self.fmsg.type.append(msgnum)
        submsg = getattr(self.fmsg, msgtype.lower()).add()
        # actual msg loading
        submsg.key = key.encode()
        # send_default_reply-ish
        raw_str = self.fmsg.SerializeToString()
        self.tprint(DEBUG, f"sending {len(raw_str)}B {self.fmsg}")
        self.req_fac.send(raw_str)
        self.msgout += 1
        self.fmsg.ParseFromString(self.req_fac.recv())
        self.msgin += 1
        # mget_status-ish
        self.tprint(DEBUG, f"got {self.fmsg}")
        index = 0
        submsg_list = getattr(self.fmsg, msgtype.lower() + "_ret")
        result = int(submsg_list[index].result)
        key_r = submsg_list[index].key
        if result == VMI_FAILURE:
            raise Exception("response received, but failure occured")
        if not key == key_r:
            raise Exception("response received, but keys did not match")
        value = str(submsg_list[index].value).decode()
        return value

    # ---------------------------------------------

    def event_register(self, edata):
        """
        Supported API call.
        Register a callback for a certain event.
        :param edata: A dict containing registration info, including the following key:value pairs.
            'event_type': Event type constant, see constants.py.
            'callback': Tenant function pointer to call when matching event arrives.
            'sync': SYNC|ASYNC constant, if SYNC, Furnace will attempt to pause the guest while the tenant app processes the event.  If ASYNC, the guest will be resumed immediately, likely before the tenant app has seen the event.
            if event_type == TIMER, also include these key:value pairs:
                'time_value': (float) call this callback every X seconds.
            if event_type == REG, also include these key:value pairs:
                'reg_type': Register type constant (e.g., .fe.CR3), see constants.py.
            if event_type == BP, also include these key:value pairs:
                'bp_pid': (int) The pid of the address space to insturment.  0 is kernel.
                'bp_addr': (int) The address itself.
        :returns: event ID.  Can be later used to clear this event.
        :raises: Exception for unknown or unsupported event type.
        :raises: Exception for unknown sync const.
        """
        self.tprint(DEBUG, "EVENT_REGISTER")

        callback = edata.pop("callback")
        if not callable(callback):
            raise Exception("callback is not callable!")
        etype = edata.pop("event_type")
        candidate = {"event_type": etype, "callback": callback, "status": ACTIVE}

        if etype == BE:
            tid = self.next_tid
            self.next_tid += 1
            self.timerlist[tid] = candidate
            self.be_info = candidate
            return tid

        elif etype == TIMER:
            tid = self.next_tid
            self.next_tid += 1
            candidate["time_value"] = float(edata.pop("time_value"))
            candidate["last_called"] = 0.0
            self.timerlist[tid] = candidate
            return tid

        # evenything that follows is related to VMI
        sync = edata.pop("sync")
        eid = -1

        if sync not in ACCEPT_SYNC.keys():
            raise Exception("sync argument %s not in %s" % (sync, ACCEPT_SYNC))

        if etype == REG:
            reg_type = edata.pop("reg_type")
            if reg_type not in ACCEPT_REG.keys():
                raise Exception(
                    "reg_type argument %s not in %s" % (reg_type, ACCEPT_REG)
                )
            eid = int(self.event_register_cr3(ACCEPT_REG[reg_type], ACCEPT_SYNC[sync]))
            candidate["reg_type"] = reg_type
            self.eventlist[eid] = candidate
            return eid

        elif etype == INT:
            bp_pid = edata.pop("bp_pid")
            bp_addr = edata.pop("bp_addr")
            eid = int(self.event_register_int(bp_pid, bp_addr, ACCEPT_SYNC[sync]))
            candidate["bp_pid"] = bp_pid
            candidate["bp_addr"] = bp_addr
            self.eventlist[eid] = candidate
            return eid

        elif etype == MEM:
            raise Exception("not presently supported")

        else:
            raise Exception("unknown event_type")

    def event_register_int(self, bp_pid, bp_addr, sync):
        """
        Internal use only.
        Performs event registration for breakpoints.
        """
        msgtype = "EVENT_REGISTER"
        submsg = self.vmmsg_helper(msgtype)
        submsg.event_type = vmi_pb2.__getattribute__("F_INT")
        submsg.sync_state = vmi_pb2.__getattribute__(sync)
        submsg.bp_pid = bp_pid
        submsg.bp_name = "furnace"
        if bp_pid == 0:
            bp_addr = bp_addr | 0xffff000000000000
        submsg.bp_addr = bp_addr
        self.tprint(DEBUG, "Registering event\n%s" % (self.vmsg,))
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        if status == VMI_SUCCESS:
            event_id = self.vmsg.event_register_ret[0].identifier
            self.tprint(DEBUG, "Event #%s" % (event_id,))
            return int(event_id)
        else:
            raise Exception("failed to register event")

    def event_register_cr3(self, reg_type, sync):
        """
        Internal use only.
        Performs event registration for registers.
        """
        msgtype = "EVENT_REGISTER"
        submsg = self.vmmsg_helper(msgtype)
        submsg.event_type = vmi_pb2.__getattribute__("F_REG")
        submsg.reg_value = vmi_pb2.__getattribute__(reg_type)
        submsg.sync_state = vmi_pb2.__getattribute__(sync)
        self.tprint(DEBUG, "Registering event\n%s" % (self.vmsg,))
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        if status == VMI_SUCCESS:
            event_id = self.vmsg.event_register_ret[0].identifier
            self.tprint(DEBUG, "Event #%s" % (event_id,))
            return int(event_id)
        else:
            raise Exception("failed to register event")

    def event_clear(self, unsafe_eid):
        """
        Supported API call.
        Clear a callback for a certain event.
        :param tid: Event ID, returned from event_register.
        :returns: Nothing.
        :raises: Exception for an unknown event.
        :raises: Exception if the caller attempts to clear the BE callback.
        """
        msgtype = "EVENT_CLEAR"
        self.tprint(DEBUG, msgtype)
        eid = int(unsafe_eid)
        try:
            e = self.eventlist[eid]
        except KeyError:
            try:
                e = self.timerlist[eid]
            except KeyError:
                raise Exception("unknown event id: {eid}".format(eid=eid))

        if e["event_type"] == TIMER:
            self.timerlist[eid]["status"] = INACTIVE
            return True
        elif e["event_type"] == BE:
            raise Exception("cannot unregister from BE event source")

        # evenything that follows is related to VMI
        submsg = self.vmmsg_helper(msgtype)
        submsg.identifier = eid
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "Clearing event %s s=%s" % (eid, status))
        if status == VMI_SUCCESS:
            self.eventlist[eid]["status"] = INACTIVE
            return True
        else:
            raise Exception(
                "VMI partition failed to remove event {eid}".format(eid=eid)
            )

    def events_start(self):
        """
        Supported API call.
        Tells the VMI partition to send events as they occur.
        (events still need to be registered)
        """
        msgtype = "EVENTS_START"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmsg_helper(msgtype)
        submsg.status = 1
        self.send_default_reply()
        # status = self.get_status(msgtype)
        self.tprint(DEBUG, "Starting events\n")

    def events_stop(self):
        """
        Supported API call.
        Tells the VMI partition to MASK any future events.
        These events aren't removed, they still occur, but they are not sent via IPC.
        It is best to clear any events instead of using this function.
        """
        msgtype = "EVENTS_STOP"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmsg_helper(msgtype)
        submsg.status = 1
        self.send_default_reply()
        # status = self.get_status(msgtype)
        self.tprint(DEBUG, "Stopping events\n")

    def start_syscall(self, callback, sync):
        """
        Supported API call.
        Start DRAKVUF-supported syscall tracing.
        :params callback: Tenant function pointer to call when matching event arrives.
        :params sync: SYNC|ASYNC constant, if SYNC, Furnace will attempt to pause the guest while the tenant app processes the event.  If ASYNC, the guest will be resumed immediately, likely before the tenant app has seen the event.
        :returns: event ID.  Can be later used to clear this event.
        :raises: Exception if callback is not callable.
        :raises: Exception if invalid sync const is used.
        :raises: Exception VMI partition was unable to start syscall tracing.
        """
        msgtype = "START_SYSCALL"
        self.tprint(DEBUG, msgtype)
        if not callable(callback):
            raise Exception("callback is not callable!")
        if sync not in ACCEPT_SYNC.keys():
            raise Exception("sync argument %s not in %s" % (sync, ACCEPT_SYNC))

        candidate = {
            "event_type": SYSCALL_PLUGIN,
            "callback": callback,
            "status": ACTIVE,
        }
        submsg = self.vmsg_helper(msgtype)
        submsg.sync_state = vmi_pb2.__getattribute__(ACCEPT_SYNC[sync])
        self.send_default_reply()
        status = self.get_status(msgtype)
        if status == VMI_SUCCESS:
            eid = int(self.vmsg.start_syscall_ret.identifier)
            self.eventlist[eid] = candidate
            self.tprint(DEBUG, f"start syscall OK, {eid}, e={self.eventlist}\n")
            return eid
        else:
            raise Exception("start_syscall failed")

    def stop_syscall(self, unsafe_eid):
        """
        Supported API call.
        Stop syscall tracing.
        :param tid: Event ID, returned from event_register.
        :returns: Nothing.
        :raises: Exception for an unknown event.
        """
        msgtype = "STOP_SYSCALL"
        self.tprint(DEBUG, msgtype)
        eid = int(unsafe_eid)
        try:
            e = self.eventlist[eid]
        except KeyError:
            raise Exception(f"unknown event id: {eid}")
        submsg = self.vmsg_helper(msgtype)
        self.send_default_reply()
        status = self.get_status(msgtype)
        if status == VMI_SUCCESS:
            self.eventlist[eid]["status"] = INACTIVE
        return status

    def lookup_symbol(self, symbol_name):
        """
        Supported API call.
        Matches DRAKVUF behavior.
        """
        msgtype = "LOOKUP_SYMBOL"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol_name = symbol_name
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "lookup_symbol %s = %s" % (symbol_name, result))
        return int(result)

    def lookup_structure(self, structure_name, structure_key):
        """
        Supported API call.
        Matches DRAKVUF behavior.
        """
        msgtype = "LOOKUP_STRUCTURE"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.structure_name = structure_name
        submsg.structure_key = structure_key
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "lookup_structure {structure_name}/{structure_key}={result}")
        return int(result)

    def process_list(self):
        """
        Supported API call.
        Calls VMI partition's built-in process list function.
        """
        msgtype = "PROCESS_LIST"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmsg_helper(msgtype)
        self.send_default_reply()
        submsg = getattr(self.vmsg, msgtype.lower() + "_ret")
        # self.tprint('debug', submsg)
        # print(submsg[0])
        status = submsg[0].status
        if status == VMI_SUCCESS:
            pslist = {}
            for proc in submsg[0].process:
                pslist[proc.pid] = {
                    "name": proc.name,
                    "process_block_ptr": proc.process_block_ptr,
                }
        else:
            raise Exception("process_list failed")
        self.tprint(DEBUG, "process_list=%s" % (len(pslist),))
        return pslist

    def resume_vm(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "RESUME_VM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        # submsg = self.vmsg_helper(msgtype)
        submsg.status = 1
        self.send_default_reply()
        # status = self.get_status(msgtype)
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "Resuming guest %s\n" % status)
        return

    def pause_vm(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "PAUSE_VM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        # submsg = self.vmsg_helper(msgtype)
        submsg.status = 1
        self.send_default_reply()
        # status = self.get_status(msgtype)
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "Pausing guest %s\n" % status)

    def translate_kv2p(self, vaddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "TRANSLATE_KV2P"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "translate_kv2p")
        return int(result)

    def translate_uv2p(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "TRANSLATE_UV2P"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "translate_uv2p")
        return int(result)

    def translate_ksym2v(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "TRANSLATE_KSYM2V"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "translate_ksym2v %s: %s" % (symbol, result))
        return int(result)

    def read_pa(self, paddr, count):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        submsg.count = int(count)
        self.send_default_reply()
        index = 0
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        # status = int(submsg_list[index].status)
        result = bytes(submsg_list[index].result)
        bytes_read = int(submsg_list[index].bytes_read)
        self.tprint(DEBUG, "read_pa, bytes_read=%d" % (bytes_read,))
        return result

    def read_va(self, vaddr, pid, count):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.count = int(count)
        submsg.pid = int(pid)
        self.send_default_reply()
        index = 0
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        # status = int(submsg_list[index].status)
        result = bytes(submsg_list[index].result)
        bytes_read = int(submsg_list[index].bytes_read)
        self.tprint(DEBUG, "read_va, bytes_read=%d" % (bytes_read,))
        return result

    def read_ksym(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_ksym")
        return int(result)

    def read_8_pa(self, paddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_8_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_8_pa")
        return int(result)

    def read_16_pa(self, paddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_16_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_16_pa")
        return int(result)

    def read_32_pa(self, paddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_32_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_32_pa")
        return int(result)

    def read_64_pa(self, paddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_64_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_64_pa")
        return int(result)

    def read_addr_pa(self, paddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_ADDR_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_addr_pa")
        return int(result)

    def read_str_pa(self, paddr):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_STR_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_str_pa")
        return int(result)

    def read_8_va(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_8_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_8_va")
        return int(result)

    def read_16_va(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_16_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_16_va")
        return int(result)

    def read_32_va(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_32_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_32_va %s:%s = r%s, %s" % (pid, vaddr, status, result))
        return int(result)

    def read_64_va(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_64_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_64_va")
        return int(result)

    def read_str_va(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_STR_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_str_va %s:%s = r%s, %s" % (pid, vaddr, status, result))
        return result

    def read_addr_va(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_ADDR_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(
            DEBUG, "read_addr_va %s:%s = r%s, %s" % (pid, vaddr, status, result)
        )
        return int(result)

    def read_8_ksym(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_8_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_8_ksym")
        return int(result)

    def read_16_ksym(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_16_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_16_ksym")
        return int(result)

    def read_32_ksym(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_32_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_32_ksym")
        return int(result)

    def read_64_ksym(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_64_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_64_ksym")
        return int(result)

    def read_addr_ksym(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_ADDR_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_addr_ksym")
        return int(result)

    def read_str_ksym(self, symbol):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "READ_STR_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "read_str_ksym")
        return int(result)

    def write_pa(self, paddr, buf):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        submsg.buffer = buf.encode()
        submsg.count = len(submsg.buffer)
        self.send_default_reply()
        index = 0
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        # status = int(submsg_list[index].status)
        bytes_written = int(submsg_list[index].bytes_written)
        self.tprint(DEBUG, "write_pa")
        return bytes_written

    def write_va(self, vaddr, pid, buf):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        submsg.buffer = buf.encode()
        submsg.count = len(submsg.buffer)
        self.send_default_reply()
        index = 0
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        # status = int(submsg_list[index].status)
        bytes_written = int(submsg_list[index].bytes_written)
        self.tprint(DEBUG, "write_va")
        return bytes_written

    def write_ksym(self, symbol, buf):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        submsg.buffer = buf
        submsg.count = len(buf)
        self.send_default_reply()
        index = 0
        submsg_list = getattr(self.vmsg, msgtype.lower() + "_ret")
        # status = int(submsg_list[index].status)
        bytes_written = int(submsg_list[index].bytes_written)
        self.tprint(DEBUG, "write_ksym")
        return bytes_written

    def write_8_pa(self, paddr, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_8_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_8_pa")
        return status

    def write_16_pa(self, paddr, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_16_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_16_pa")
        return status

    def write_32_pa(self, paddr, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_32_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_32_pa")
        return status

    def write_64_pa(self, paddr, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_64_PA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.paddr = int(paddr)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_64_pa")
        return status

    def write_8_va(self, vaddr, pid, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_8_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_8_va")
        return status

    def write_16_va(self, vaddr, pid, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_16_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_16_va")
        return status

    def write_32_va(self, vaddr, pid, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_32_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.value = int(value)
        submsg.pid = int(pid)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_32_va")
        return status

    def write_64_va(self, vaddr, pid, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_64_VA"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_64_va")
        return status

    def write_8_ksym(self, symbol, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_8_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_8_ksym")
        return status

    def write_16_ksym(self, symbol, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_16_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_16_ksym")
        return status

    def write_32_ksym(self, symbol, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_32_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_32_ksym")
        return status

    def write_64_ksym(self, symbol, value):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "WRITE_64_KSYM"
        self.tprint(DEBUG, msgtype)
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = symbol
        submsg.value = int(value)
        self.send_default_reply()
        status = self.mget_status(msgtype, 0)
        self.tprint(DEBUG, "write_64_ksym")
        return status

    def get_name(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_NAME"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.tprint(DEBUG, self.vmsg)
        self.send_default_reply()
        self.tprint(DEBUG, self.vmsg)
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_name %s" % (result,))
        return result

    def get_vmid(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_VMID"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_vmid")
        return result

    def get_access_mode(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_ACCESS_MODE"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_access_mode")
        return result

    def get_winver(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_WINVER"
        submsg = self.vmmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype)
        self.tprint(DEBUG, "get_winver")
        return result

    def get_winver_str(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_WINVER_STR"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_winver_str")
        return result

    def get_vcpureg(self, vaddr, pid):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_VCPUREG"
        submsg = self.vmmsg_helper(msgtype)
        submsg.vaddr = int(vaddr)
        submsg.pid = int(pid)
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "get_vcpureg")
        return int(result)

    def get_memsize(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_MEMSIZE"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_memsize")
        return result

    def get_offset(self, offset):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_OFFSET"
        submsg = self.vmmsg_helper(msgtype)
        submsg.symbol = offset
        self.send_default_reply()
        status, result = self.mget_status_result(msgtype, 0)
        self.tprint(DEBUG, "get_offset %s: %s" % (offset, result))
        return int(result)

    def get_ostype(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_OSTYPE"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_ostype")
        return result

    def get_page_mode(self):
        """
        Supported API call.
        Matches LibVMI behavior.
        """
        msgtype = "GET_PAGE_MODE"
        submsg = self.vmsg_helper(msgtype)
        submsg.placeholder = 1
        self.send_default_reply()
        status, result = self.get_status_result(msgtype)
        self.tprint(DEBUG, "get_page_mode")
        return result

    def log(self, data):
        """
        Supported API call.
        Write to underlying logging mechanism.
        :param data: The data.
        :returns: Nothing.
        """
        self.tprint(INFO, f"APP: {data}")

    def set_name(self, name):
        """
        Supported API call.
        :param name: The app's name (must be alphanum and <=16 characters).
        :returns: Nothing.
        :raises: Exception if malformed.
        """
        if 0 < len(name) < 17 and re.match("^[\w-]+$", name):
            self.name = name
            self.tprint(DEBUG, "setting app name to %s" % (name,))
            return True
        else:
            self.tprint(ERROR, "app name must be alphanum,-,_ and 0 < len(name) < 17")
            return False

    def exit(self):
        """
        Supported API call.
        Exit without tearing down.
        :returns: Nothing.
        """
        self.tprint(DEBUG, "exit called, flushing queues")
        # for some reason this hangs the sandbox, commenting out for now
        # time.sleep(1)  # clear msg queues
        self.tprint(DEBUG, "calling exit")
        sys.exit()

    def shutdown(self):
        """
        Supported API call.
        Exit cleanly.
        :returns: Nothing.
        """
        if self.already_shutdown:
            self.tprint(ERROR, "already shutdown?")
            return

        self.already_shutdown = True
        self.tprint(WARN, "shutting down")
        try:
            self.tprint(WARN, "shutting down app")
            self.app.shutdown()
        except:
            self.tprint(ERROR, "error during app shutdown")
            pass

        self.poller.unregister(self.dealer_vmi)
        self.req_vmi.close()
        self.dealer_vmi.close()

        self.poller.unregister(self.dealer_fac)
        self.req_fac.close()
        self.dealer_fac.close()

        self.context.destroy()
        super(FE, self).shutdown()
