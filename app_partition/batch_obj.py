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
Batch_obj class.  Allows tenants to pipeline VMI calls to the VMI partition.
The tenant programmer creates a batch object that they then fill with VMI calls.
The batch object puts data directly into a protobuf.
The tenant sends batch object into the api, which takes the protobuf and sends it off to the VMI partition.
The tenant takes the same object, and gets results out of a generator.

Example usage to extract process list from VM:

o = fe.batch_new()
o.add('000', 'PAUSE_VM', {'status': 1})
o.add('01', 'READ_STR_VA', {'vaddr': cur+nameo-tasko, 'pid': 0})
o.add('02', 'READ_32_VA', {'vaddr': cur+pido-tasko, 'pid': 0})
o.add('03', 'READ_ADDR_VA', {'vaddr': cur, 'pid': 0})
for i in range(1, 120):
    o.add(f'{i}1', 'READ_STR_VA', {'vaddr': nameo-tasko, 'pid': 0}, f'{i-1}3', 'result', 'vaddr', fe.ADD)
    o.add(f'{i}2', 'READ_32_VA', {'vaddr': pido-tasko, 'pid': 0}, f'{i-1}3', 'result', 'vaddr', fe.ADD)
    o.add(f'{i}3', 'READ_ADDR_VA', {'vaddr': 0, 'pid': 0}, f'{i-1}3', 'result', 'vaddr', fe.ADD)
o.add('001', 'RESUME_VM', {'status': 1})
o.serialize()

fe.batch_send(self.o)

for r in o.results():
    print(r)
"""

import string

from constants import *
import vmi_pb2

EMPTY = 0
FILLING = 1
SERIALIZED = 2
RESULTS_RAW = 3
RESULTS_PARSED = 4


class batch_obj(object):
    """
    Batch_obj class.  Tenant programmer creates one of these for each pipelined message.
    """

    def __init__(self):
        """
        Constructor.  Initializes internal state.
        """
        # self.Result = collections.namedtuple('Result', 'name status result')
        self.request = vmi_pb2.VMIMessage()
        self.reply = vmi_pb2.VMIMessage()
        self.reset()

    def reset(self):
        """
        Supported API call.
        Resets the object's internal state.  Could be used to re-use the object for more than one call.
        :returns: Nothing.
        """
        self.request.Clear()
        self.reply.Clear()
        self.namemap = {}  # map of name -> submsg
        self.order = []  # list of submsg numbers in order
        self.namestack = []  # list of names in order
        self.raw_out = ""
        self.raw_in = ""
        self.state = EMPTY

    def add(
        self,
        name,
        msgtype,
        argv,
        linkname=None,
        linkfield=None,
        linktgt=None,
        linkopr=None,
    ):
        """
        Supported API call.
        Adds a single VMI call to the pipeline.  Optionally, link this call to the results of an earlier call.
        :param name: The label for this VMI call.  Can be used to reference it later (in results).
        :param msgtype: (string) VMI call to perform.  Supports most (all?) VMI API calls.  Eventually matched against the name in the proto format (vmi.proto).
        :param argv: A dict of arguments to pass to the VMI call.  Argument names are matched against the name in the proto format.
        message Read_32_va {
            optional uint64 vaddr = 1;
            optional int32 pid = 2;
            optional Linkage link = 3;
        }
        For example, above is the message format for read_32_va().  Here, a valid argv would be {'vaddr': 0x1234, 'pid': 0}
        The final four parameters allow the caller to reference results of VMI calls earlier in the pipeline.
        :param linkname: The label for the preceeding call to reference.
        :param linkfield: The name of the field to reference (will usually be 'result').
        :param linktgt: The name of this VMI call's argument to place the referenced value.
        :param linkopr: The mathematical operator to use when combining in the referenced value.  Options:
            fe.ADD, actual_arg = current_arg + referenced
            fe.SUB, actual_arg = current_arg - referenced
            fe.OVERWRITE, actual_arg = referenced
        :returns: Nothing.
        """
        # print(f'add({name}, {msgtype}, {argv}, linkname={linkname}, linkfield={linkfield}, linktgt={linktgt}, linkopr={linkopr})')

        msgnum = self.request.__getattribute__(msgtype)
        self.request.type.append(msgnum)
        submsg = getattr(self.request, msgtype.lower()).add()
        for argname, argv in argv.items():
            setattr(submsg, argname, argv)

        if linkname and linkfield and linktgt and linkopr:
            tgtmsg = self.namemap[linkname]  # the message we're going to link to
            tgtmsgix = self.namestack.index(linkname)  # it is the nth message
            tgtmsg_name = tgtmsg.DESCRIPTOR.name.lower()
            # print(f'tgtmsg_name={tgtmsg_name}')
            tgtmsg_type = vmi_pb2.VMIMessage.DESCRIPTOR.fields_by_name[
                tgtmsg_name
            ].number
            # print(f'tgtmsg_type={tgtmsg_type}')
            tgtmsg_ret_type = (
                tgtmsg_type + 1
            )  # the reply message type vmi will be hunting for
            # print(f'tgtmsg_ret_type={tgtmsg_ret_type}')
            tgtmsg_ret = getattr(
                vmi_pb2,
                self.request.DESCRIPTOR.fields_by_number[
                    tgtmsg_ret_type
                ].name.capitalize(),
            )  # this grabs the ret submsg
            # print(f'tgtmsg_ret={tgtmsg_ret.DESCRIPTOR.name}')
            # print(f'looking for {linkfield}')
            tgtfieldix = tgtmsg_ret.DESCRIPTOR.fields_by_name[
                linkfield
            ].number  # the field index within this message
            # print(f'tgtfieldix={tgtfieldix}')
            myfieldix = submsg.DESCRIPTOR.fields_by_name[
                linktgt
            ].number  # the current message's field
            submsg.link.prev_msg_ix = tgtmsgix
            submsg.link.prev_item_ix = (
                tgtfieldix - 1
            )  # we get 0 indexed, but tgtfieldix is 1 indexed
            submsg.link.cur_item_ix = (
                myfieldix - 1
            )  # we get 0 indexed, but myfieldix is 1 indexed
            submsg.link.operator = linkopr

        self.namemap[name] = submsg
        self.namestack.append(name)
        self.order.append(msgnum)
        self.state = FILLING
        # print(f'added:\n{submsg}')

        """
    def runfake(self):
        print(self.request)
        self.reply = self.request
        return True
"""

    def results_size(self):
        """
        Supported API call.
        Peers into the results protobuf and makes an educated guess at its size.
        :returns: Size of results.
        """
        if self.state == RESULTS_RAW:
            self.parse()
        return len(self.reply.type)

    def serialize(self):
        """
        Supported API call.
        Serializes the protobuf.  Will happen automatically if needed.
        :returns: Nothing.
        """
        self.state = SERIALIZED
        self.raw_out = self.request.SerializeToString()

    def parse(self):
        """
        Supported API call.
        Parses the results protobuf.  Will happen automatically if needed.
        :returns: Nothing.
        """
        self.reply.ParseFromString(self.raw_in)
        if len(self.request.type) != len(self.reply.type):
            raise Exception(
                "something is wrong, out: %d, in: %d"
                % (len(self.request.type), len(self.reply.type))
            )
        if self.reply.error:
            raise Exception(
                "got error: num:%d, loc:%d"
                % (self.reply.error_num, self.reply.error_loc)
            )
        self.state = RESULTS_PARSED

    def results(self):
        """
        Supported API call.
        A generator that returns VMI call results in the order they were sent.
        :returns: Each iteration results a results dict.  Example:
        {'name': [label], 'result': 0x1234, 'status': 0}
        """
        typecount = {}
        i = 0
        if self.state == RESULTS_RAW:
            self.parse()
        for t in self.reply.type:
            ix = typecount.get(t, 0)
            typecount[t] = ix + 1

            fname = vmi_pb2.VMIMessage.DESCRIPTOR.fields_by_number[t].name
            submsg_list = getattr(self.reply, fname.lower())
            submsg = submsg_list[ix]

            # start to rebuild message
            res = {}
            md = getattr(vmi_pb2, string.capwords(fname))

            res["name"] = self.namestack[i]
            for j in md.DESCRIPTOR.fields:
                if j.name == "link":
                    continue
                res[j.name] = getattr(submsg, j.name)
            i += 1

            yield res


if __name__ == "__main__":

    list_head = 1234

    o = batch_obj()
    o.add("01", "READ_STR_VA", {"vaddr": list_head + 400, "pid": 0})
    o.add("02", "READ_32_VA", {"vaddr": list_head + 140, "pid": 0})
    o.add("03", "READ_ADDR_VA", {"vaddr": list_head, "pid": 0})
    for i in range(1, 10):
        o.add(
            f"{i}1",
            "READ_STR_VA",
            {"vaddr": 400, "pid": 0},
            f"{i-1}3",
            "pid",
            "vaddr",
            F_ADD,
        )
        o.add(
            f"{i}2",
            "READ_32_VA",
            {"vaddr": 140, "pid": 0},
            f"{i-1}3",
            "pid",
            "vaddr",
            F_ADD,
        )
        o.add(
            f"{i}3",
            "READ_ADDR_VA",
            {"vaddr": 0, "pid": 0},
            f"{i-1}3",
            "pid",
            "vaddr",
            F_ADD,
        )

    o.runfake()

    print("RESULTS:")
    for r in o.results():
        print(r)
