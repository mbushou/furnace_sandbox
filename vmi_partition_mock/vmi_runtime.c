/*
 * Furnace (c) 2017-2018 Micah Bushouse
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <inttypes.h>
#include <signal.h>

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <glib.h>
#include <err.h>

#include <zmq.h>
#include "vmi.pb.h"

#define MOCK_DRIVER 1
#define LIBVMI_DRIVER 2
#define DRAKVUF_DRIVER 3

#define DRIVER MOCK_DRIVER
//#define DRIVER LIBVMI_DRIVER

#if DRIVER == LIBVMI_DRIVER or DRIVER == DRAKVUF_DRIVER
//#include <libvmi/libvmi.h>
//#include <libvmi/events.h>
#include "libvmi_driver.h"

extern "C" {
#define XC_WANT_COMPAT_EVTCHN_API 1
#include <xenctrl.h>
#include <xenctrl_compat.h>
}
#elif DRIVER == MOCK_DRIVER
#include "mock_driver.h"
#endif

// cd f_vmi_v4
// protoc --proto_path=../proto --cpp_out=. ../proto/vmi.proto
// g++ -g -lprotobuf -c vmi.pb.cc
// export PKG_CONFIG_PATH=/home/micah/current_libvmi
//
// LIBVMI:
// g++ -g -Wno-write-strings -Wall `pkg-config --cflags --libs glib-2.0 libvmi` -lpthread -lzmq -lprotobuf -lxenctrl -lxenlight -lxentoollog libvmi_driver.c vmi.pb.o vmi_runtime.c -o vmi_runtime
// sudo LD_PRELOAD=/usr/local/lib/libvmi.so.0.0.13 ./vmi_runtime fg25-1
//
// MOCK:
// g++ -g -Wno-write-strings -Wall `pkg-config --cflags --libs glib-2.0` -lpthread -lzmq -lprotobuf -lxenctrl -lxenlight -lxentoollog libvmi_mock.c vmi.pb.o mock_driver.c vmi_runtime.c -o vmi_runtime
// ./vmi_runtime fg25-1
//

#ifdef DO_FURNACE_PROFILE
#define FURNACE_PROFILE(s)    P_RDTSC(s)
static inline void P_RDTSC(const char* s)
{
    unsigned int hi, lo;
    __asm__ volatile("rdtsc": "=a" (lo), "=d" (hi));
    fprintf(stderr, "%s, %lu\n", s, ((uint64_t) hi << 32) | lo);
}
#else
#define FURNACE_PROFILE(s)
#endif

#define DO_FURNACE_DEBUG

#ifdef DO_FURNACE_DEBUG
#define FURNACE_DEBUG(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define FURNACE_DEBUG(fmt, args...)
#endif

/* ----------------------------------------------------- */
/* globals */

class event_item
{
public:
    uint64_t id;
    int sync_state;
    bool active;
    std::string fe_source;
    vmi_event_t* event;
    //drakvuf_trap_t *event;
    //furnace *furn;
    char* owner;
};

vmi_instance_t vmi;

FurnaceProto::VMIMessage msg_in;
FurnaceProto::VMIMessage msg_out;

std::map<uint32_t, uint64_t> cache;  // link cache
uint32_t nth_type;      // the type "stack," one entry per message, best speedup potential
uint32_t nth_field_ix;  // the index within the field type, good speedup potential
uint32_t msgno;         // the current message being processed

uint64_t msg_ctr;
uint64_t call_ctr;
uint64_t async_send_ctr;

int max_events = 200;

std::string rekall_profile;
std::string vm_name;
std::string pub_path;
std::string rtr_path;

char* scratch_ident;
void* zsock_rtr;
void* zcontext;
std::map<uint64_t, event_item*> events;
std::map<uint64_t, event_item*>::iterator it;
bool events_active;


// REQ -- RTR
void zmq_msend(char* ident, std::string r)
{
    FURNACE_DEBUG("SENDING M %luB to %s\n", r.length(), ident);
    int rc = zmq_send(zsock_rtr, ident, strlen(ident), ZMQ_SNDMORE);
    assert(rc == (int) strlen(ident));
    rc = zmq_send(zsock_rtr, "", 0, ZMQ_SNDMORE);
    assert(rc == 0);
    rc = zmq_send(zsock_rtr, r.c_str(), r.length(), 0);
    assert(rc == (int) r.length());
    return;
}

// DEALER - RTR
void zmq_dsend(char* ident, std::string r)
{
    FURNACE_DEBUG("SENDING D %luB to %s\n", r.length(), ident);
    int rc = zmq_send(zsock_rtr, ident, strlen(ident), ZMQ_SNDMORE);
    assert(rc == (int) strlen(ident));
    rc = zmq_send(zsock_rtr, r.c_str(), r.length(), 0);
    assert(rc == (int) r.length());
    return;
}

void
batch_error(int index,
            FurnaceProto::VMIMessage* msg_out)
{
    FURNACE_DEBUG("- finishing batch error\n");
    msg_out->set_error_loc(index);
    return;
}

void
set_batch_error(int error_num,
                FurnaceProto::VMIMessage* msg_out)
{
    FURNACE_DEBUG("- setting batch error\n");
    msg_out->set_error(true);
    msg_out->set_error_num(error_num);
    return;
}

bool
process_link(google::protobuf::Message* m,
             const FurnaceProto::Linkage* link,
             int index,
             FurnaceProto::VMIMessage* msg_in,
             FurnaceProto::VMIMessage* msg_out)
{
    FURNACE_PROFILE("STLINK");
    FURNACE_DEBUG("Doing Linkage processing:\n");
    FURNACE_DEBUG("- link->prev_msg_ix: %u\n", link->prev_msg_ix());
    FURNACE_DEBUG("- link->prev_item_ix: %u\n", link->prev_item_ix());
    FURNACE_DEBUG("- link->cur_item_ix: %u\n", link->cur_item_ix());
    FURNACE_DEBUG("- link->operator: %d\n", (int) link->operator_());
    FURNACE_DEBUG("- msgno: %u\n", msgno);
    FURNACE_DEBUG("- nth_type: %u\n", nth_type);
    //FURNACE_DEBUG("- nth_field: %u\n", nth_field);
    FURNACE_DEBUG("- nth_field_ix: %u\n", nth_field_ix);

    // it is not allowed to reference "forward" in the chain, only backwards
    if (msgno <= nth_type)
    {
        FURNACE_DEBUG("error: asking for ix:%d from msgno:%d\n", link->prev_msg_ix(), msgno);
        set_batch_error(10, msg_out);
        return false;
    }

    // input validation: the number of fields in this msg cannot be less
    // than the desired index.
    if ((uint32_t) msg_in->type_size() < link->prev_msg_ix())
    {
        FURNACE_DEBUG("error: asking for ix:%d from msg with size:%d\n", msg_in->type_size(), link->prev_msg_ix());
        set_batch_error(11, msg_out);
        return false;
        //assert(msg_in->type_size() >= link->prev_msg_ix());
    }
    unsigned int msgtype = msg_in->type(link->prev_msg_ix());

    // 0. initialization
    FURNACE_DEBUG("- inferring ret from protobuf numbering (msgtype+1)\n");
    msgtype += 1;
    const google::protobuf::Descriptor* desc = msg_out->GetDescriptor();
    if (desc == NULL)
    {
        set_batch_error(12, msg_out);
        return false;
        //assert(desc != NULL);
    }
    const google::protobuf::Reflection* refl = msg_out->GetReflection();
    if (refl == NULL)
    {
        set_batch_error(13, msg_out);
        return false;
        //assert(refl != NULL);
    }
    // 0. need to find the type of the message at position prev_msg_ix.
    // this will be stored in tgttype
    std::vector<const google::protobuf::FieldDescriptor*> output;
    refl->ListFields(*msg_out, &output);
    FURNACE_DEBUG("- looking for msg #%d of type %d\n", link->prev_msg_ix(), msgtype);

    uint64_t prior_result = 0;

    // warning: we assume we'll use only _one_ result field per message
    if (cache.find(link->prev_msg_ix()) == cache.end())
    {
        // not in the cache
        const google::protobuf::FieldDescriptor* tgtfield = NULL;
        int d = -1;
        const google::protobuf::Descriptor* ifd = NULL;
        const google::protobuf::Reflection* ir = NULL;

        unsigned int tgttype = 0;  // for a field, it's number in the message
        unsigned int tgtdepth = index;  // for a repeated value, it's location in the field

        // just go throught the types first, finding the type at the index we care
        // about, then using 'index' to determine the actual target based on depth
        for (auto i=output.rbegin(); i!=output.rend(); i++)    // note reverse iteration to get type first
        {
            unsigned int fieldsz = refl->FieldSize(*msg_out, (*i));
            FURNACE_DEBUG("- type field ix=%d %s %s/%d with size %d\n", d,
                          (*i)->full_name().c_str(), (*i)->type_name(), (*i)->number(), fieldsz);
            FURNACE_DEBUG("- starting from location %d\n", nth_type);
            FURNACE_DEBUG("- type list (%d) ", (*i)->number());

            for (unsigned int j=nth_type; j<fieldsz; j++)
            {
                //for (unsigned int j=0; j<fieldsz; j++) {
                int t = refl->GetRepeatedEnumValue(*msg_out, (*i), j);
                FURNACE_DEBUG("%d  ", t);
                if (j == link->prev_msg_ix())
                {
                    tgttype = t;
                    nth_type = j;
                    FURNACE_DEBUG("* (%u)", j);
                    break;
                }
            }
            FURNACE_DEBUG("\n");
            break;
        }

        d = -1;
        //d = nth_field;
        FURNACE_DEBUG("- starting from field %d\n", d);

        //for (auto i=output.rbegin() + nth_field; i!=output.rend(); i++){  // note reverse iteration to get type first
        for (auto i=output.rbegin(); i!=output.rend(); i++)   // note reverse iteration to get type first
        {
            unsigned int fieldsz = refl->FieldSize(*msg_out, (*i));
            FURNACE_DEBUG("- found field ix %d:%s of type %s/%d and size %d\n", d, (*i)->full_name().c_str(), (*i)->type_name(), (*i)->number(), fieldsz);
            // our actual target could a repeated value (deep) inside this field
            if (d == -1)
            {
                FURNACE_DEBUG("- skipping type field\n");
                d++;
                continue;
            }
            if ((unsigned int)(*i)->number() == tgttype)    // this is the field we want
            {
                //if (d + fieldsz > link->prev_msg_ix()) {
                FURNACE_DEBUG("- types this: %d, desired: %d\n", (*i)->number(), msgtype);

                // :%s/msg_in->\(\w\+\)(index)/m/gc
                // dig into each repeated field until we find our target
                // this is where a lot of iterations occur

                FURNACE_DEBUG("- starting from field ix %u\n", nth_field_ix);
                for (unsigned int j=nth_field_ix; j<fieldsz; j++)
                {
                    //for (unsigned int j=0; j<fieldsz; j++) {
                    FURNACE_DEBUG("- moving down the list d:%d, j:%d\n", d, j);
                    if ((unsigned int)tgtdepth == j+d)
                    {
                        //if (link->prev_msg_ix() == j+d){
                        FURNACE_DEBUG("- prev_msg_ix says we're here! d:%d, j:%d\n", d, j);
                        const google::protobuf::Message& tgtmsg = refl->GetRepeatedMessage(*msg_out, (*i), j);
                        ifd = tgtmsg.GetDescriptor();
                        if (ifd == NULL)
                        {
                            set_batch_error(14, msg_out);
                            return false;
                            //assert(ifd != NULL);
                        }
                        ir = tgtmsg.GetReflection();
                        if (ir == NULL)
                        {
                            set_batch_error(15, msg_out);
                            return false;
                            //assert(ir != NULL);
                        }
                        tgtfield = *i;

                        if (tgtfield == NULL)
                        {
                            printf("could not make sense of prev_msg_ix: %d\n", link->prev_msg_ix());
                            set_batch_error(16, msg_out);
                            return false;
                            //assert(tgtfield != NULL);
                        }
                        tgttype = tgtfield->number();
                        FURNACE_DEBUG("- this field's tag (and type) are %d\n", tgttype);
                        if (tgttype != msgtype)
                        {
                            set_batch_error(17, msg_out);
                            return false;
                            //assert(tgttype == msgtype);
                        }

                        // input validation: the number of fields in this msg cannot be less
                        // than the desired index.
                        if ((uint32_t) ifd->field_count() < link->prev_item_ix())
                        {
                            set_batch_error(18, msg_out);
                            return false;
                            //assert(ifd->field_count() >= link->prev_item_ix());
                        }
                        const google::protobuf::FieldDescriptor* imsgfield = ifd->field(link->prev_item_ix());
                        if (imsgfield == NULL)
                        {
                            set_batch_error(19, msg_out);
                            return false;
                            //assert(imsgfield != NULL);
                        }
                        FURNACE_DEBUG("- prior msg field #%d has type %s\n", link->prev_item_ix(), imsgfield->type_name());
                        switch (imsgfield->type())
                        {
                            case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                                FURNACE_DEBUG("- uint64 switch statement\n");
                                prior_result = ir->GetUInt64(tgtmsg, imsgfield);
                                break;
                            case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                                FURNACE_DEBUG("- int32 switch statement\n");
                                prior_result = (uint64_t) ir->GetInt32(tgtmsg, imsgfield);
                                break;
                            case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                                FURNACE_DEBUG("- uint32 switch statement\n");
                                prior_result = (uint64_t) ir->GetUInt32(tgtmsg, imsgfield);
                                break;
                            default:
                                printf("non-compatible type!\n");
                                set_batch_error(20, msg_out);
                                return false;
                                //assert(0);
                                break;
                        }
                        FURNACE_DEBUG("- got prior_result %lu\n", prior_result);
                        //nth_field = d;
                        nth_field_ix = j;
                        break;
                    }
                }
            }
            d++;
        }

        cache[link->prev_msg_ix()] = prior_result;
        FURNACE_PROFILE("CSLINK");
        FURNACE_DEBUG("- cache being loaded\n");
    }
    else
    {
        prior_result = cache[link->prev_msg_ix()];
        FURNACE_PROFILE("CALINK");
        FURNACE_DEBUG("- cache hit, msg #%d found: %lu\n", link->prev_msg_ix(), prior_result);
    }
    FURNACE_DEBUG("- cache size: %lu\n", cache.size());


    // 4. get the type and value from the current message
    //const google::protobuf::Message& cur_msg = refl->GetRepeatedMessage(*msg_out, msgfield, link->prev_item_ix());
    const google::protobuf::Descriptor* mfd = m->GetDescriptor();
    if (mfd == NULL)
    {
        set_batch_error(21, msg_out);
        return false;
        //assert(mfd != NULL);
    }
    // input validation: the number of fields in this msg cannot be less
    // than the desired index.
    if ((uint32_t) mfd->field_count() < link->cur_item_ix())
    {
        set_batch_error(22, msg_out);
        return false;
        //assert(mfd->field_count() >= link->cur_item_ix());
    }
    const google::protobuf::FieldDescriptor* mmsgfield = mfd->field(link->cur_item_ix());
    if (mmsgfield == NULL)
    {
        set_batch_error(23, msg_out);
        return false;
        //assert(mmsgfield != NULL);
    }
    FURNACE_DEBUG("- current msg field #%d has type %s\n", link->prev_item_ix(), mmsgfield->type_name());
    const google::protobuf::Reflection* mr = m->GetReflection();
    if (mr == NULL)
    {
        set_batch_error(24, msg_out);
        return false;
        //assert(mr != NULL);
    }
    uint64_t current_input = 0;
    switch (mmsgfield->type())
    {
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
            FURNACE_DEBUG("- uint64 switch statement\n");
            current_input = mr->GetUInt64(*m, mmsgfield);
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
            FURNACE_DEBUG("- int32 switch statement\n");
            current_input = (uint64_t) mr->GetInt32(*m, mmsgfield);
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
            FURNACE_DEBUG("- uint32 switch statement\n");
            current_input = (uint64_t) mr->GetUInt32(*m, mmsgfield);
            break;
        default:
            printf("non-compatible type!\n");
            //assert(0);
            set_batch_error(25, msg_out);
            return false;
            break;
    }
    FURNACE_DEBUG("- got current_input %lu\n", current_input);

    // 5. sanity check to make sure types match
    // disabling this for now, since these will be caught in the preceeding sw
    // statements
    /*
    if (imsgfield->type() != mmsgfield->type()) {
      printf("error, types don't match, %d -> %d\n", imsgfield->type(), mmsgfield->type());
      assert(0);
    }*/

    // 6. operate on the two values and store the result
    uint64_t newval = 0;
    switch (link->operator_())
    {
        case FurnaceProto::Linkage_OperatorType::Linkage_OperatorType_F_ADD:
            FURNACE_DEBUG("- adding: %lu + %lu\n", current_input, prior_result);
            newval = current_input + prior_result;
            break;
        case FurnaceProto::Linkage_OperatorType::Linkage_OperatorType_F_SUB:
            FURNACE_DEBUG("- subtracting: %lu - %lu\n", current_input, prior_result);
            if (current_input < prior_result)
            {
                printf("subtraction is invalid!  would result in a negative number\n");
                if (current_input < prior_result)
                {
                    set_batch_error(26, msg_out);
                    return false;
                    //assert(current_input > prior_result);
                }
            }
            newval = current_input - prior_result;
            break;
        case FurnaceProto::Linkage_OperatorType::Linkage_OperatorType_F_OVERWRITE:
            FURNACE_DEBUG("- overwrite: %lu with %lu\n", current_input, prior_result);
            newval = prior_result;
            break;
        default:
            printf("invalid operator!\n");
            set_batch_error(27, msg_out);
            return false;
            //assert(0);
            break;
    }
    switch (mmsgfield->type())
    {
        case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
            FURNACE_DEBUG("- uint64 switch set statement\n");
            mr->SetUInt64(m, mmsgfield, newval);
            break;
        case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
            FURNACE_DEBUG("- int32 switch set statement\n");
            mr->SetInt32(m, mmsgfield, (int32_t) newval);
            break;
        default:
            printf("non-compatible type!\n");
            set_batch_error(28, msg_out);
            return false;
            //assert(0);
            break;
    }
    FURNACE_DEBUG("- newval: %lu\n", newval);
    FURNACE_PROFILE("FNLINK");
    return true;
}


#if DRIVER == LIBVMI_DRIVER or DRIVER == DRAKVUF_DRIVER
/*
reg_t cr3;
vmi_event_t cr3_event;

event_response_t cr3_all_tasks_callback(vmi_instance_t vmi, vmi_event_t* event)
{
    printf("CR3=%" PRIx64 " executing on vcpu %" PRIu32 ". Previous CR3=%" PRIx64 "\n",
           event->reg_event.value, event->vcpu_id, event->reg_event.previous);

    zmq_dsend("testclient", std::to_string(event->reg_event.value));

    return 0;
}
*/
#endif


void
process_read_8_va(int index,
                  FurnaceProto::VMIMessage* msg_in,
                  FurnaceProto::VMIMessage* msg_out)
{
    uint8_t res = 0;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_8_va(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_pid())
    {
        pid = m.pid();
    }
    else
    {
        FURNACE_DEBUG("read_8_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_8_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_8_va_ret();
    if (st == VMI_SUCCESS)
    {
        FURNACE_DEBUG("read_8_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        st = fdrive_read_8_va(vmi, vaddr, pid, &res);  // locking up??
        FURNACE_DEBUG("read_8_va: st=%d, res=%d\n", st, res);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_VA_RET);
    return;
}

void
process_read_16_va(int index,
                   FurnaceProto::VMIMessage* msg_in,
                   FurnaceProto::VMIMessage* msg_out)
{
    uint16_t res = 0;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_16_va(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_pid())
    {
        pid = m.pid();
    }
    else
    {
        FURNACE_DEBUG("read_16_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_16_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_16_va_ret();
    if (st == VMI_SUCCESS)
    {
        FURNACE_DEBUG("read_16_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        st = fdrive_read_16_va(vmi, vaddr, pid, &res);
        FURNACE_DEBUG("read_16_va: st=%d, res=%u\n", st, res);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_VA_RET);
    return;
}

void
process_read_32_va(int index,
                   FurnaceProto::VMIMessage* msg_in,
                   FurnaceProto::VMIMessage* msg_out)
{
    uint32_t res = 0;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_32_va(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_pid())
    {
        pid = m.pid();
    }
    else
    {
        FURNACE_DEBUG("read_32_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_32_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_32_va_ret();
    if (st == VMI_SUCCESS)
    {
        FURNACE_DEBUG("read_32_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        st = fdrive_read_32_va(vmi, vaddr, pid, &res);
        FURNACE_DEBUG("read_32_va: st=%d, res=%u\n", st, res);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_VA_RET);
    return;
}

void
process_read_64_va(int index,
                   FurnaceProto::VMIMessage* msg_in,
                   FurnaceProto::VMIMessage* msg_out)
{
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_64_va(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_pid())
    {
        pid = m.pid();
    }
    else
    {
        FURNACE_DEBUG("read_64_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_64_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_64_va_ret();
    if (st == VMI_SUCCESS)
    {
        FURNACE_DEBUG("read_64_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        st = fdrive_read_64_va(vmi, vaddr, pid, &res);
        FURNACE_DEBUG("read_64_va: st=%d, res=%lu\n", st, res);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_VA_RET);
    return;
}



void
process_single(FurnaceProto::VMIMessage_Type msg_type,
               int index,
               FurnaceProto::VMIMessage* msg_in,
               FurnaceProto::VMIMessage* msg_out)
{

    call_ctr++;
    FURNACE_DEBUG("process_single: t=%d, i=%d, cc=%lu\n", msg_type, index, call_ctr);
    switch (msg_type)
    {
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_VA:
            process_read_8_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_VA:
            process_read_16_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_VA:
            process_read_32_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_VA:
            process_read_64_va(index, msg_in, msg_out);
            break;

        default:
            // invalid type, generic error message
            msg_out->set_error_loc(index + 10000);  // +10000 signals error happened here
            msg_out->set_error(true);
            msg_out->set_error_num(msg_type);
            printf("error in switch process_single: t=%d, i=%d\n", msg_type, index);
            break;
    }
    return;
}

std::string
process_protobuf(char* buf, int len)
{

    FurnaceProto::VMIMessage_Type msg_type;
    std::map<FurnaceProto::VMIMessage_Type, int> typecount;
    std::string r;
    //const std::string sbuf = buf;

    FURNACE_PROFILE("PBINIT");
    if (msg_in.ParseFromArray(buf, len) == false)
    {
        printf("FURNACE: could not parse incoming protobuf\n");
    }
    msg_out.Clear();
    cache.clear();  // clear the link cache from the prior message
    nth_type = 0;
    nth_field_ix = 0;
    msgno = 0;
    FURNACE_DEBUG("FURNACE: message (%dB) has %d types\n", len, msg_in.type_size());

    if (msg_in.type_size() > 1000)
    {
        FURNACE_DEBUG("FURNACE: Error, too many messages (%d)\n", msg_in.type_size());

    }
    else
    {

        for (int i=0; i<msg_in.type_size(); i++)
        {
            msg_type = msg_in.type(i);
            FURNACE_DEBUG("%d: %d\n", i, msg_type);

            if (typecount.find(msg_type) == typecount.end())
            {
                typecount[msg_type] = 0;
            }
            else
            {
                typecount[msg_type]++;
            }
            if (msg_out.has_error())
            {
                printf("got error at message %d, will not process further messages\n", i);
                break;
            }
            FURNACE_PROFILE("STSIGL");
            process_single(msg_type, typecount[msg_type], &msg_in, &msg_out);
            msgno++;
            FURNACE_PROFILE("FNSIGL");
        }
    }

    msg_out.SerializeToString(&r);
    FURNACE_PROFILE("PBSERL");
    FURNACE_DEBUG("FURNACE: sending message (%luB)\n", r.length());
    return r;
}

int process_zmq()
{
    uint32_t zevents = 0;
    size_t zevents_s = sizeof(zevents);
    int rc;
    char* ident;

    do
    {
        rc = zmq_getsockopt(zsock_rtr, ZMQ_EVENTS, &zevents, &zevents_s);
        assert(rc == 0);
        if (zevents & ZMQ_POLLIN)
        {

            zmq_msg_t zmsg;
            rc = zmq_msg_init(&zmsg);

            int part = 1;
            do
            {
                rc = zmq_msg_recv(&zmsg, zsock_rtr, ZMQ_DONTWAIT);
                FURNACE_DEBUG("FURNACE: received message part %d of size %dB\n", part, rc);

                /*
                if (part != 2) {
                  FURNACE_DEBUG("FURNACE: part != 2 message part %d of size %dB\n", part, rc);
                  assert(rc > 0); // can be == 0 because of null message part
                }
                */

                char* buf = (char*) zmq_msg_data(&zmsg);
                std::string r;

                switch (part)
                {
                    case 1:
                        ident = strndup(buf, rc);
                        scratch_ident = ident;
                        FURNACE_DEBUG("identity size=%d: %s\n", rc, ident);
                        break;
                    case 2:
                        break;
                    case 3:
                        msg_ctr++;
                        FURNACE_DEBUG("process_protobuf size=%d: %s\n", rc, ident);
                        r = process_protobuf(buf, rc);
                        zmq_msend(ident, r);
                        zmq_msend(ident, "hello!");
                        break;
                    default:
                        printf("ERROR, part=%d\n", part);
                        break;
                }
                part++;
            }
            while (zmq_msg_more(&zmsg));

            rc = zmq_msg_close(&zmsg);
            free(ident);

        }
    }
    while (zevents & ZMQ_POLLIN);

    return 0;
}



static int interrupted = 0;
static void close_handler(int sig)
{
    interrupted = sig;
}

int main (int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    //vmi_instance_t vmi = NULL;  // now a global
    status_t status = VMI_SUCCESS;
    int rc = 0;

    struct sigaction act;

    char* name = NULL;

    if (argc < 2)
    {
        printf("bad usage\n");
        exit(1);
    }

    // Arg 1 is the VM name.
    name = argv[1];

    /* for a clean exit */
    act.sa_handler = close_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGALRM, &act, NULL);

#if DRIVER == LIBVMI_DRIVER or DRIVER == DRAKVUF_DRIVER
    xc_interface* xci = (xc_interface*) xc_interface_open(NULL, NULL, 0);
    xc_evtchn* xec = xc_evtchn_open(NULL, 0);
    int xec_fd = xc_evtchn_fd(xec);
#elif DRIVER == MOCK_DRIVER
    //void* xci = NULL;
    void* xec = NULL;
    //int xec_fd = 400;
#endif

    // zmq start
    printf("zmq\n");
    //std::string rtr_path = "/tmp/test_vmi_runtime";
    std::string rtr_path = "/opt/furnace/vmi_socket/vmi_rtr";
    //std::string rtr_path = "/opt/furnace/f_tenant_data/321-321-321-vmi/vmi_socket/vmi_rtr";
    std::string rtr_uri = "ipc://" + rtr_path;

    zcontext = zmq_ctx_new();
    zmq_ctx_set(zcontext, ZMQ_IO_THREADS, 6);

    zsock_rtr = zmq_socket(zcontext, ZMQ_ROUTER);
    rc = zmq_bind(zsock_rtr, rtr_uri.c_str());
    assert(rc == 0);

    FURNACE_DEBUG("- chmoding %s\n", rtr_uri.c_str());
    char mode[] = "0777";
    int i = strtol(mode, 0, 8);
    rc = chmod(rtr_path.c_str(), i);
    assert(rc == 0);

    //zsock_rtr = zmq_socket(zcontext, ZMQ_REP);
    //rc = zmq_bind(zsock_rtr, "tcp://127.0.0.1:5555");
    //assert (rc == 0);

    printf("getsockopt up\n");
    int fd_1;
    size_t fd_len = sizeof(fd_1);
    rc = zmq_getsockopt(zsock_rtr, ZMQ_FD, &fd_1, &fd_len);
    assert (rc == 0);
    assert (fd_len > 0);

    printf("pollfds\n");

    int ni = 2;
    struct pollfd pfds[2];
    pfds[0].fd = fd_1;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
#if DRIVER == LIBVMI_DRIVER or DRIVER == DRAKVUF_DRIVER
    pfds[1].fd = xec_fd;
    pfds[1].events = POLLIN | POLLERR;
    pfds[1].revents = 0;
#elif DRIVER == MOCK_DRIVER
    ni = 1;
#endif

    printf("priming zmq fd\n");
    int timeout = 0;
    size_t timeout_len = sizeof(timeout);
    rc = zmq_setsockopt(zsock_rtr, ZMQ_RCVTIMEO, &timeout, timeout_len);
    assert(rc == 0);

    char tmp [10];
    rc = zmq_recv(zsock_rtr, tmp, 10, 0);
    assert(rc == -1);
    assert(errno & EAGAIN);
    // zmq end

    /*
    // single vmi poll
    int ni = 1;
    struct pollfd pfds;
    pfds.fd = xec_fd;
    pfds.events = POLLIN | POLLERR;
    pfds.revents = 0;
    */

    int poll_timeout_msecs = 1000;

    // vmi
    if (VMI_FAILURE ==
            fdrive_init_complete(&vmi, name,
                                 VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS | VMI_INIT_XEN_EVTCHN,
                                 (void*) xec, VMI_CONFIG_GLOBAL_FILE_ENTRY, NULL, NULL))
    {
        printf("Failed to init LibVMI library.\n");
        return 1;
    }
    printf("LibVMI init succeeded!\n");

    /*
    memset(&cr3_event, 0, sizeof(vmi_event_t));
    cr3_event.version = VMI_EVENTS_VERSION;
    cr3_event.type = VMI_EVENT_REGISTER;
    cr3_event.reg_event.reg = CR3;
    cr3_event.reg_event.in_access = VMI_REGACCESS_W;
    cr3_event.callback = cr3_all_tasks_callback;

    printf("registering event\n");
    vmi_register_event(vmi, &cr3_event);
    */

    //int i = 0;
    i = 0;
    int k = 0;
    while (!interrupted)
    {
        //char buffer [10];
        uint32_t zevents = 0;
        size_t zevents_s = sizeof(zevents);

        printf(".");

        rc = poll(pfds, ni, poll_timeout_msecs);

        if (rc == 0)
        {
            printf(".");
            continue;
        }
        else if (rc < 0)
        {
            printf("poll error\n");
            break;
        }

        printf("%d poll rc=%d\n", k, rc);

        // pfds[0] == zmq, pfds[1] == vmi
        for (i=0; i<ni; i++)
        {
            printf("%d revents=%d\n", i, pfds[i].revents);
        }

        // zmq
        if (pfds[0].revents & POLLIN)
        {
            do
            {
                rc = zmq_getsockopt(zsock_rtr, ZMQ_EVENTS, &zevents, &zevents_s);
                assert(rc == 0);
                if (zevents > 0)
                {

                    printf("%d ZMQ_EVENTS=%d\n", i, zevents);

                    if (zevents & ZMQ_POLLIN)
                    {
                        printf("main->process_zmq()\n");
                        process_zmq();
                    }
                }
            }
            while (zevents & ZMQ_POLLIN);
        }

        // vmi
        if (pfds[1].revents & POLLIN)
        {
            status = fdrive_events_listen(vmi, 1);
            if (status != VMI_SUCCESS)
            {
                printf("error! breaking out of while loop\n");
                interrupted = -1;
            }
        }

    }

    printf("Finished with test.\n");

    fdrive_destroy(vmi);
#if DRIVER == LIBVMI_DRIVER or DRIVER == DRAKVUF_DRIVER
    xc_interface_close(xci);
#endif

    return 0;
}
