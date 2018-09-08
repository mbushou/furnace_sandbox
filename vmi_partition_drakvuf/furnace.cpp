/*********************IMPORTANT DRAKVUF LICENSE TERMS***********************
 *                                                                         *
 * DRAKVUF (C) 2014-2017 Tamas K Lengyel.                                  *
 * Tamas K Lengyel is hereinafter referred to as the author.               *
 * This program is free software; you may redistribute and/or modify it    *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; Version 2 ("GPL"), BUT ONLY WITH ALL OF THE   *
 * CLARIFICATIONS AND EXCEPTIONS DESCRIBED HEREIN.  This guarantees your   *
 * right to use, modify, and redistribute this software under certain      *
 * conditions.  If you wish to embed DRAKVUF technology into proprietary   *
 * software, alternative licenses can be aquired from the author.          *
 *                                                                         *
 * Note that the GPL places important restrictions on "derivative works",  *
 * yet it does not provide a detailed definition of that term.  To avoid   *
 * misunderstandings, we interpret that term as broadly as copyright law   *
 * allows.  For example, we consider an application to constitute a        *
 * derivative work for the purpose of this license if it does any of the   *
 * following with any software or content covered by this license          *
 * ("Covered Software"):                                                   *
 *                                                                         *
 * o Integrates source code from Covered Software.                         *
 *                                                                         *
 * o Reads or includes copyrighted data files.                             *
 *                                                                         *
 * o Is designed specifically to execute Covered Software and parse the    *
 * results (as opposed to typical shell or execution-menu apps, which will *
 * execute anything you tell them to).                                     *
 *                                                                         *
 * o Includes Covered Software in a proprietary executable installer.  The *
 * installers produced by InstallShield are an example of this.  Including *
 * DRAKVUF with other software in compressed or archival form does not     *
 * trigger this provision, provided appropriate open source decompression  *
 * or de-archiving software is widely available for no charge.  For the    *
 * purposes of this license, an installer is considered to include Covered *
 * Software even if it actually retrieves a copy of Covered Software from  *
 * another source during runtime (such as by downloading it from the       *
 * Internet).                                                              *
 *                                                                         *
 * o Links (statically or dynamically) to a library which does any of the  *
 * above.                                                                  *
 *                                                                         *
 * o Executes a helper program, module, or script to do any of the above.  *
 *                                                                         *
 * This list is not exclusive, but is meant to clarify our interpretation  *
 * of derived works with some common examples.  Other people may interpret *
 * the plain GPL differently, so we consider this a special exception to   *
 * the GPL that we apply to Covered Software.  Works which meet any of     *
 * these conditions must conform to all of the terms of this license,      *
 * particularly including the GPL Section 3 requirements of providing      *
 * source code and allowing free redistribution of the work as a whole.    *
 *                                                                         *
 * Any redistribution of Covered Software, including any derived works,    *
 * must obey and carry forward all of the terms of this license, including *
 * obeying all GPL rules and restrictions.  For example, source code of    *
 * the whole work must be provided and free redistribution must be         *
 * allowed.  All GPL references to "this License", are to be treated as    *
 * including the terms and conditions of this license text as well.        *
 *                                                                         *
 * Because this license imposes special exceptions to the GPL, Covered     *
 * Work may not be combined (even as part of a larger work) with plain GPL *
 * software.  The terms, conditions, and exceptions of this license must   *
 * be included as well.  This license is incompatible with some other open *
 * source licenses as well.  In some cases we can relicense portions of    *
 * DRAKVUF or grant special permissions to use it in other open source     *
 * software.  Please contact tamas.k.lengyel@gmail.com with any such       *
 * requests.  Similarly, we don't incorporate incompatible open source     *
 * software into Covered Software without special permission from the      *
 * copyright holders.                                                      *
 *                                                                         *
 * If you have any questions about the licensing restrictions on using     *
 * DRAKVUF in other works, are happy to help.  As mentioned above,         *
 * alternative license can be requested from the author to integrate       *
 * DRAKVUF into proprietary applications and appliances.  Please email     *
 * tamas.k.lengyel@gmail.com for further information.                      *
 *                                                                         *
 * If you have received a written license agreement or contract for        *
 * Covered Software stating terms other than these, you may choose to use  *
 * and redistribute Covered Software under those terms instead of these.   *
 *                                                                         *
 * Source is provided to this software because we believe users have a     *
 * right to know exactly what a program is going to do before they run it. *
 * This also allows you to audit the software for security holes.          *
 *                                                                         *
 * Source code also allows you to port DRAKVUF to new platforms, fix bugs, *
 * and add new features.  You are highly encouraged to submit your changes *
 * on https://github.com/tklengyel/drakvuf, or by other methods.           *
 * By sending these changes, it is understood (unless you specify          *
 * otherwise) that you are offering unlimited, non-exclusive right to      *
 * reuse, modify, and relicense the code.  DRAKVUF will always be          *
 * available Open Source, but this is important because the inability to   *
 * relicense code has caused devastating problems for other Free Software  *
 * projects (such as KDE and NASM).                                        *
 * To specify special license conditions of your contributions, just say   *
 * so when you send them.                                                  *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the DRAKVUF   *
 * license file for more details (it's in a COPYING file included with     *
 * DRAKVUF, and also available from                                        *
 * https://github.com/tklengyel/drakvuf/COPYING)                           *
 *                                                                         *
 ***************************************************************************/

#include <string>
#include <limits>

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <dirent.h>
#include <glib.h>
#include <err.h>

#include <zmq.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <libvmi/libvmi.h>

//#include "vmi.pb.h"
#include "furnace.h"
#include "private.h"
#include "../syscalls/syscalls.h"

/*
static inline uint64_t RDTSC() {
    unsigned int hi, lo;
    __asm__ volatile("rdtsc": "=a" (lo), "=d" (hi));
    return ((uint64_t) hi << 32) | lo;
}
*/

//#define DO_FURNACE_PROFILE

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

void furnace_zmq_cb(int fd, void* data)
{
    PRINT_DEBUG("furnace_zmq_cb with data=%" PRIx64 "\n", (addr_t) data);
    furnace* f = (furnace*) data;
    f->process_zmq(f->zsock_rtr);
    PRINT_DEBUG("furnace_zmq_cb done\n");
}

void zmq_hop(void* f, void* zsock)
{
    FURNACE_PROFILE("ZMQHOP");
    furnace* furn = (furnace*) f;
    furn->process_zmq(zsock);
    FURNACE_PROFILE("FNDONE");
}

// REQ -- RTR
void zmq_msend(void* zsock, char* ident, std::string r)
{
    FURNACE_DEBUG("SENDING M %luB to %s\n", r.length(), ident);
    int rc = zmq_send(zsock, ident, strlen(ident), ZMQ_SNDMORE);
    assert(rc == (int) strlen(ident));
    rc = zmq_send(zsock, "", 0, ZMQ_SNDMORE);
    assert(rc == 0);
    rc = zmq_send(zsock, r.c_str(), r.length(), 0);
    assert(rc == (int) r.length());
    return;
}

// DEALER - RTR
void zmq_dsend(void* zsock, char* ident, std::string r)
{
    FURNACE_DEBUG("SENDING D %luB to %s\n", r.length(), ident);
    int rc = zmq_send(zsock, ident, strlen(ident), ZMQ_SNDMORE);
    assert(rc == (int) strlen(ident));
    rc = zmq_send(zsock, r.c_str(), r.length(), 0);
    assert(rc == (int) r.length());
    return;
}

void furnace_syscall_callback(void* f,
                              drakvuf_trap_info_t* info)
{
    FURNACE_DEBUG("SYSCALL CALLBACK\n");
    furnace* furn = (furnace*) f;

    FurnaceProto::VMIMessage msg;
    std::string r;
    switch (furn->syscall_plugin_sync_state)
    {
        case FurnaceProto::F_SYNC:
            drakvuf_pause(furn->drakvuf);  // must explicitly turned back on later?
            msg.mutable_event()->set_guest_paused(true);
            break;
        case FurnaceProto::F_ASYNC:
        default:
            msg.mutable_event()->set_guest_paused(false);
            break;
    }
    msg.add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT);
    msg.mutable_event()->set_identifier(furn->syscall_plugin_event_id);
    msg.mutable_event()->set_cr3(info->regs->cr3);
    msg.mutable_event()->set_rip(info->regs->rip);
    msg.mutable_event()->set_rbp(info->regs->rbp);
    msg.mutable_event()->set_rsp(info->regs->rsp);
    msg.mutable_event()->set_rdi(info->regs->rdi);
    msg.mutable_event()->set_rsi(info->regs->rsi);
    msg.mutable_event()->set_rdx(info->regs->rdx);
    msg.mutable_event()->set_rax(info->regs->rax);
    msg.mutable_event()->set_vcpu(info->vcpu);
    msg.mutable_event()->set_uid(info->proc_data.userid);
    msg.mutable_event()->set_procname(info->proc_data.name);
    msg.mutable_event()->set_pid(info->proc_data.pid);
    msg.mutable_event()->set_ppid(info->proc_data.ppid);
    msg.SerializeToString(&r);
    furn->async_send_ctr++;
    zmq_dsend(furn->zsock_rtr, furn->syscall_plugin_owner, r);
    return;
}

static event_response_t furnace_cr3_callback(drakvuf_t drakvuf, drakvuf_trap_info_t* info)
{
    event_item* ei = (event_item*)info->trap->data;
    furnace* furn = ei->furn;
    if (furn->events_active == false || ei->active == false)
    {
        return 0;
    }
    //FURNACE_DEBUG("CR3 CR3=0x%" PRIx64 "\n", info->regs->cr3);

    FurnaceProto::VMIMessage msg;
    std::string r;
    switch (ei->sync_state)
    {
        case FurnaceProto::F_SYNC:
            FURNACE_DEBUG("in cr3 callback: pausing guest\n");
            drakvuf_pause(drakvuf);  // must explicitly turned back on later?
            msg.mutable_event()->set_guest_paused(true);
            break;
        case FurnaceProto::F_ASYNC:
        default:
            msg.mutable_event()->set_guest_paused(false);
            break;
    }
    msg.add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT);
    msg.mutable_event()->set_identifier(ei->id);
    msg.mutable_event()->set_cr3(info->regs->cr3);
    msg.mutable_event()->set_rip(info->regs->rip);
    msg.mutable_event()->set_rbp(info->regs->rbp);
    msg.mutable_event()->set_rsp(info->regs->rsp);
    msg.mutable_event()->set_rdi(info->regs->rdi);
    msg.mutable_event()->set_rsi(info->regs->rsi);
    msg.mutable_event()->set_rdx(info->regs->rdx);
    msg.mutable_event()->set_rax(info->regs->rax);
    msg.mutable_event()->set_vcpu(info->vcpu);
    msg.mutable_event()->set_uid(info->proc_data.userid);
    msg.mutable_event()->set_procname(info->proc_data.name);
    msg.mutable_event()->set_pid(info->proc_data.pid);
    msg.mutable_event()->set_ppid(info->proc_data.ppid);
    msg.SerializeToString(&r);
    furn->async_send_ctr++;
    zmq_dsend(furn->zsock_rtr, ei->owner, r);

    return 0;
}

static event_response_t furnace_int_callback(drakvuf_t drakvuf, drakvuf_trap_info_t* info)
{
    event_item* ei = (event_item*)info->trap->data;
    furnace* furn = ei->furn;
    if (furn->events_active == false || ei->active == false)
    {
        return 0;
    }
    //FURNACE_DEBUG("INT CR3=0x%" PRIx64 "\n", info->regs->cr3);

    FurnaceProto::VMIMessage msg;
    std::string r;
    switch (ei->sync_state)
    {
        case FurnaceProto::F_SYNC:
            FURNACE_DEBUG("Pausing guest\n");
            drakvuf_pause(drakvuf);  // must explicitly turned back on later?
            msg.mutable_event()->set_guest_paused(true);
            break;
        case FurnaceProto::F_ASYNC:
        default:
            msg.mutable_event()->set_guest_paused(false);
            break;
    }
    msg.add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT);
    msg.mutable_event()->set_identifier(ei->id);
    msg.mutable_event()->set_cr3(info->regs->cr3);
    msg.mutable_event()->set_rip(info->regs->rip);
    msg.mutable_event()->set_rbp(info->regs->rbp);
    msg.mutable_event()->set_rsp(info->regs->rsp);
    msg.mutable_event()->set_rdi(info->regs->rdi);
    msg.mutable_event()->set_rsi(info->regs->rsi);
    msg.mutable_event()->set_rdx(info->regs->rdx);
    msg.mutable_event()->set_rax(info->regs->rax);
    msg.mutable_event()->set_vcpu(info->vcpu);
    msg.mutable_event()->set_uid(info->proc_data.userid);
    msg.mutable_event()->set_procname(info->proc_data.name);
    msg.mutable_event()->set_pid(info->proc_data.pid);
    msg.mutable_event()->set_ppid(info->proc_data.ppid);
    msg.SerializeToString(&r);
    furn->async_send_ctr++;
    zmq_dsend(furn->zsock_rtr, ei->owner, r);

    return 0;
}

/*
static event_response_t furnace_ept_callback(drakvuf_t drakvuf, drakvuf_trap_info_t *info) {
  event_item *ei = (event_item *)info->trap->data;
  furnace *furn = ei->furn;
  if (furn->events_active == false || ei->active == false) {
    return 0;
  }
  FURNACE_DEBUG("EPT RIP=0x%" PRIx64 " CR3=0x%" PRIx64 "\n", info->regs->rip, info->regs->cr3);

  FurnaceProto::VMIMessage msg;
  std::string r;
  switch (ei->sync_state) {
  case FurnaceProto::F_SYNC:
    FURNACE_DEBUG("Pausing guest\n");
    drakvuf_pause(drakvuf);  // must explicitly turned back on later?
    msg.mutable_event()->set_guest_paused(true);
    break;
  case FurnaceProto::F_ASYNC:
  default:
    msg.mutable_event()->set_guest_paused(false);
    break;
  }
  msg.add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT);
  msg.mutable_event()->set_identifier(ei->id);
  msg.mutable_event()->set_cr3(info->regs->cr3);
  msg.mutable_event()->set_rip(info->regs->rip);
  msg.mutable_event()->set_rbp(info->regs->rbp);
  msg.mutable_event()->set_rsp(info->regs->rsp);
  msg.mutable_event()->set_rdi(info->regs->rdi);
  msg.mutable_event()->set_rsi(info->regs->rsi);
  msg.mutable_event()->set_rdx(info->regs->rdx);
  msg.mutable_event()->set_rax(info->regs->rax);
  msg.SerializeToString(&r);
  furn->async_send_ctr++;
  zmq_dsend(furn->zsock_rtr, ei->owner, r);

  return 0;
}
*/

/* ----------------------------------------------------- */

void
furnace::batch_error(int index,
                     FurnaceProto::VMIMessage* msg_out)
{
    FURNACE_DEBUG("- finishing batch error\n");
    msg_out->set_error_loc(index);
    return;
}

void
furnace::set_batch_error(int error_num,
                         FurnaceProto::VMIMessage* msg_out)
{
    FURNACE_DEBUG("- setting batch error\n");
    msg_out->set_error(true);
    msg_out->set_error_num(error_num);
    return;
}

bool
furnace::process_link(google::protobuf::Message* m,
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
    FURNACE_DEBUG("- msgno: %u\n", this->msgno);
    FURNACE_DEBUG("- nth_type: %u\n", this->nth_type);
    //FURNACE_DEBUG("- nth_field: %u\n", this->nth_field);
    FURNACE_DEBUG("- nth_field_ix: %u\n", this->nth_field_ix);

    // it is not allowed to reference "forward" in the chain, only backwards
    if (this->msgno <= this->nth_type)
    {
        FURNACE_DEBUG("error: asking for ix:%d from msgno:%d\n", link->prev_msg_ix(), this->msgno);
        this->set_batch_error(10, msg_out);
        return false;
    }

    // input validation: the number of fields in this msg cannot be less
    // than the desired index.
    if ((uint32_t) msg_in->type_size() < link->prev_msg_ix())
    {
        FURNACE_DEBUG("error: asking for ix:%d from msg with size:%d\n", msg_in->type_size(), link->prev_msg_ix());
        this->set_batch_error(11, msg_out);
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
        this->set_batch_error(12, msg_out);
        return false;
        //assert(desc != NULL);
    }
    const google::protobuf::Reflection* refl = msg_out->GetReflection();
    if (refl == NULL)
    {
        this->set_batch_error(13, msg_out);
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
    if (this->cache.find(link->prev_msg_ix()) == this->cache.end())
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
            FURNACE_DEBUG("- starting from location %d\n", this->nth_type);
            FURNACE_DEBUG("- type list (%d) ", (*i)->number());

            for (unsigned int j=this->nth_type; j<fieldsz; j++)
            {
                //for (unsigned int j=0; j<fieldsz; j++) {
                int t = refl->GetRepeatedEnumValue(*msg_out, (*i), j);
                FURNACE_DEBUG("%d  ", t);
                if (j == link->prev_msg_ix())
                {
                    tgttype = t;
                    this->nth_type = j;
                    FURNACE_DEBUG("* (%u)", j);
                    break;
                }
            }
            FURNACE_DEBUG("\n");
            break;
        }

        d = -1;
        //d = this->nth_field;
        FURNACE_DEBUG("- starting from field %d\n", d);

        //for (auto i=output.rbegin() + this->nth_field; i!=output.rend(); i++){  // note reverse iteration to get type first
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

                FURNACE_DEBUG("- starting from field ix %u\n", this->nth_field_ix);
                for (unsigned int j=this->nth_field_ix; j<fieldsz; j++)
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
                            this->set_batch_error(14, msg_out);
                            return false;
                            //assert(ifd != NULL);
                        }
                        ir = tgtmsg.GetReflection();
                        if (ir == NULL)
                        {
                            this->set_batch_error(15, msg_out);
                            return false;
                            //assert(ir != NULL);
                        }
                        tgtfield = *i;

                        if (tgtfield == NULL)
                        {
                            printf("could not make sense of prev_msg_ix: %d\n", link->prev_msg_ix());
                            this->set_batch_error(16, msg_out);
                            return false;
                            //assert(tgtfield != NULL);
                        }
                        tgttype = tgtfield->number();
                        FURNACE_DEBUG("- this field's tag (and type) are %d\n", tgttype);
                        if (tgttype != msgtype)
                        {
                            this->set_batch_error(17, msg_out);
                            return false;
                            //assert(tgttype == msgtype);
                        }

                        // input validation: the number of fields in this msg cannot be less
                        // than the desired index.
                        if ((uint32_t) ifd->field_count() < link->prev_item_ix())
                        {
                            this->set_batch_error(18, msg_out);
                            return false;
                            //assert(ifd->field_count() >= link->prev_item_ix());
                        }
                        const google::protobuf::FieldDescriptor* imsgfield = ifd->field(link->prev_item_ix());
                        if (imsgfield == NULL)
                        {
                            this->set_batch_error(19, msg_out);
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
                                this->set_batch_error(20, msg_out);
                                return false;
                                //assert(0);
                                break;
                        }
                        FURNACE_DEBUG("- got prior_result %lu\n", prior_result);
                        //this->nth_field = d;
                        this->nth_field_ix = j;
                        break;
                    }
                }
            }
            d++;
        }

        this->cache[link->prev_msg_ix()] = prior_result;
        FURNACE_PROFILE("CSLINK");
        FURNACE_DEBUG("- cache being loaded\n");
    }
    else
    {
        prior_result = this->cache[link->prev_msg_ix()];
        FURNACE_PROFILE("CALINK");
        FURNACE_DEBUG("- cache hit, msg #%d found: %lu\n", link->prev_msg_ix(), prior_result);
    }
    FURNACE_DEBUG("- cache size: %lu\n", this->cache.size());


    // 4. get the type and value from the current message
    //const google::protobuf::Message& cur_msg = refl->GetRepeatedMessage(*msg_out, msgfield, link->prev_item_ix());
    const google::protobuf::Descriptor* mfd = m->GetDescriptor();
    if (mfd == NULL)
    {
        this->set_batch_error(21, msg_out);
        return false;
        //assert(mfd != NULL);
    }
    // input validation: the number of fields in this msg cannot be less
    // than the desired index.
    if ((uint32_t) mfd->field_count() < link->cur_item_ix())
    {
        this->set_batch_error(22, msg_out);
        return false;
        //assert(mfd->field_count() >= link->cur_item_ix());
    }
    const google::protobuf::FieldDescriptor* mmsgfield = mfd->field(link->cur_item_ix());
    if (mmsgfield == NULL)
    {
        this->set_batch_error(23, msg_out);
        return false;
        //assert(mmsgfield != NULL);
    }
    FURNACE_DEBUG("- current msg field #%d has type %s\n", link->prev_item_ix(), mmsgfield->type_name());
    const google::protobuf::Reflection* mr = m->GetReflection();
    if (mr == NULL)
    {
        this->set_batch_error(24, msg_out);
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
            this->set_batch_error(25, msg_out);
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
                this->set_batch_error(26, msg_out);
                return false;
            }
            newval = current_input - prior_result;
            break;
        case FurnaceProto::Linkage_OperatorType::Linkage_OperatorType_F_OVERWRITE:
            FURNACE_DEBUG("- overwrite: %lu with %lu\n", current_input, prior_result);
            newval = prior_result;
            break;
        default:
            printf("invalid operator!\n");
            this->set_batch_error(27, msg_out);
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
            this->set_batch_error(28, msg_out);
            return false;
            //assert(0);
            break;
    }
    FURNACE_DEBUG("- newval: %lu\n", newval);
    FURNACE_PROFILE("FNLINK");
    return true;
}

/* ----------------------------------------------------- */
/* "built-ins" */

void
furnace::process_process_list(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    status_t st = VMI_SUCCESS;
    addr_t list_head = 0, cur_list_entry = 0, next_list_entry = 0;
    addr_t current_process = 0;
    char* procname = NULL;
    vmi_pid_t pid = 0;
    unsigned long tasks_offset = 0, pid_offset = 0, name_offset = 0;

    FURNACE_DEBUG("process_list\n");
    //int calls = 0;

    vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    st = vmi_translate_ksym2v(vmi, "init_task", &list_head);
    vmi_get_offset(vmi, "linux_tasks", &tasks_offset);
    vmi_get_offset(vmi, "linux_name", &name_offset);
    vmi_get_offset(vmi, "linux_pid", &pid_offset);

    list_head += tasks_offset;
    cur_list_entry = list_head;

    st = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
    //calls += 1;

    auto ret = msg_out->add_process_list_ret();
    while (st == VMI_SUCCESS)
    {

        current_process = cur_list_entry - tasks_offset;
        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);
        //calls += 1;
        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);
        //calls += 1;

        auto proc = ret->add_process();
        proc->set_pid(pid);
        proc->set_name(procname);
        proc->set_process_block_ptr(current_process);
        FURNACE_DEBUG("[%5d] %s (struct addr:%" PRIx64 ")\n", pid, procname, current_process);

        cur_list_entry = next_list_entry;
        st = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
        //calls += 1;

        if (cur_list_entry == list_head)
        {
            //printf("calls: %d\n", calls);
            break;
        }
    }
    drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("process_list done\n");

    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_PROCESS_LIST_RET);
}

void
furnace::process_start_syscall(int index,
                               FurnaceProto::VMIMessage* msg_in,
                               FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    auto ret = msg_out->mutable_start_syscall_ret();

    // Basically, we're going to treat the syscall_plugin as an event,
    // but with most of the unnecessary details empty, since these only pertain
    // to "normal" events registered through event_register().
    if (this->syscall_plugin_owner == NULL)
    {
        event_item* ei = (event_item*) malloc(sizeof(event_item));
        if (ei == NULL)
        {
            printf("Failed to allocate trap memory!\n");
            this->set_batch_error(1, msg_out);
            this->batch_error(index, msg_out);
            return;
        }
        memset(ei, 0, sizeof(event_item));
        ei->id = std::numeric_limits<uint64_t>::max();  // the actual id is set inside add_event
        ei->sync_state = msg_in->start_syscall().sync_state(); // all this stuff is redundant, syscalls are a special case
        ei->active = true;
        ei->furn = NULL;
        ei->event = NULL;
        ei->owner = NULL;
        add_event(ei, &this->events);
        this->syscall_plugin_ei = ei;
        this->syscall_plugin_event_id = ei->id;
        ret->set_identifier(ei->id);

        FURNACE_DEBUG("start_syscall, sync_state=%d\n", msg_in->start_syscall().sync_state());
        drakvuf_pause(this->drakvuf);
        this->syscall_plugin = new syscalls(this->drakvuf, this->rekall_profile.c_str(), this, &furnace_syscall_callback);
        this->syscall_plugin_sync_state = msg_in->start_syscall().sync_state();
        this->syscall_plugin_owner = strdup(this->scratch_ident);
        this->syscall_plugin_owner[strlen(this->syscall_plugin_owner)-1] = 'd';
        drakvuf_resume(this->drakvuf);

        FURNACE_DEBUG("start_syscall, done\n");
    }
    else
    {
        FURNACE_DEBUG("start_syscall error, plugin already in use by %s\n", this->syscall_plugin_owner);
        this->set_batch_error(2, msg_out);
        this->batch_error(index, msg_out);
        st = VMI_FAILURE;
        ret->set_identifier(0);
    }

    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_START_SYSCALL_RET);
}

void
furnace::process_stop_syscall(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    auto ret = msg_out->mutable_stop_syscall_ret();

    if (this->syscall_plugin_owner == NULL)
    {
        FURNACE_DEBUG("stop_syscall error, plugin not active\n");
        this->set_batch_error(1, msg_out);
        this->batch_error(index, msg_out);
        st = VMI_FAILURE;
    }
    else
    {
        FURNACE_DEBUG("stop_syscall\n");
        assert(is_event_present(this->syscall_plugin_event_id, &this->events));
        drakvuf_pause(this->drakvuf);
        FURNACE_DEBUG("Found %lu in events\n", this->syscall_plugin_event_id);
        FURNACE_DEBUG("removing from event map\n");
        this->events.erase(this->syscall_plugin_event_id);
        FURNACE_DEBUG("freeing event_item\n");
        free(this->syscall_plugin_ei);
        FURNACE_DEBUG("After deletion, map has %lu entries\n", events.size());
        FURNACE_DEBUG("deleting this->syscall_plugin\n");
        delete this->syscall_plugin;
        free(this->syscall_plugin_owner);
        this->syscall_plugin_owner = NULL;
        drakvuf_resume(this->drakvuf);
        FURNACE_DEBUG("stop_syscall, done\n");
    }

    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_STOP_SYSCALL_RET);
}

/*
void
furnace::process_start_vmifs(int index,
    FurnaceProto::VMIMessage *msg_in,
    FurnaceProto::VMIMessage *msg_out) {
  FURNACE_DEBUG("start_vmifs\n");
  status_t st = VMI_SUCCESS;
  auto ret = msg_out->mutable_start_vmifs_ret();

  if (this->vmifs_pid != -1) {
    FURNACE_DEBUG("VMIFS should already be running at %d\n", this->vmifs_pid);
    st = VMI_FAILURE;
  } else {

    // VMIFS arguments
    char *child_argv[5];
    child_argv[0] = (char*) this->vmifs_bin_path.c_str();
    child_argv[1] = (char*) "name";
    child_argv[2] = (char*) this->vm_name.c_str();
    child_argv[3] = (char*) this->vmifs_mount_path.c_str();
    child_argv[4] = NULL;

    FURNACE_DEBUG("%d forking\n", getpid());
    pid_t pid = fork();

    if (!pid) {
      FURNACE_DEBUG("child execvp-ing %d\n", getpid());
      FURNACE_DEBUG("%s %s %s %s %s\n",
          this->vmifs_bin_path.c_str(),
          child_argv[0],
          child_argv[1],
          child_argv[2],
          child_argv[3]
          );
      execvp(this->vmifs_bin_path.c_str(), child_argv);
    }
    //int status = 0;
    //int r = 0;
    //waitpid(pid, &status, 0);
    //if (WIFEXITED(status)) {
      //r = WEXITSTATUS(status);
      //PRINT_DEBUG("child exited with rc=%d\n", r);
    //}
    this->vmifs_pid = pid;
    st = VMI_SUCCESS;
  }

  ret->set_status(st);
  msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_START_VMIFS_RET);
}

void
furnace::process_stop_vmifs(int index,
    FurnaceProto::VMIMessage *msg_in,
    FurnaceProto::VMIMessage *msg_out) {
  status_t st = VMI_SUCCESS;
  FURNACE_DEBUG("stop_vmifs\n");
  auto ret = msg_out->mutable_stop_vmifs_ret();

  ret->set_status(st);
  msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_STOP_VMIFS_RET);
}
*/

/* ----------------------------------------------------- */

void
furnace::process_translate_kv2p(int index,
                                FurnaceProto::VMIMessage* msg_in,
                                FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    uint64_t vaddr = 0;
    if (msg_in->translate_kv2p(index).has_vaddr())
    {
        vaddr = msg_in->translate_kv2p(index).vaddr();
    }
    else
    {
        FURNACE_DEBUG("translate_kv2p: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_translate_kv2p_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_translate_kv2p(vmi, vaddr, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("translate_kv2p: vaddr=%lu\n", vaddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_TRANSLATE_KV2P_RET);
}

void
furnace::process_translate_uv2p(int index,
                                FurnaceProto::VMIMessage* msg_in,
                                FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->translate_uv2p(index);
    if (m.has_pid())
    {
        pid = m.pid();
    }
    else
    {
        FURNACE_DEBUG("translate_uv2p: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("translate_uv2p: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_translate_uv2p_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_translate_uv2p(vmi, vaddr, pid, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("translate_uv2p: pid=%d, vaddr=%lu\n", pid, vaddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_TRANSLATE_UV2P_RET);
    return;
}

void
furnace::process_translate_ksym2v(int index,
                                  FurnaceProto::VMIMessage* msg_in,
                                  FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->translate_ksym2v(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("translate_ksym2v: missing symbol\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_translate_ksym2v_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_translate_ksym2v(vmi, (const char*) symbol.c_str(), &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("translate_ksym2v: symbol=%s\n", symbol.c_str());
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_TRANSLATE_KSYM2V_RET);
    return;
}

void
furnace::process_read_pa(int index,
                         FurnaceProto::VMIMessage* msg_in,
                         FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* res = NULL;
    status_t st = VMI_SUCCESS;
    size_t count = 0;
    size_t bytes_read = 0;
    uint64_t paddr = 0;
    auto m = msg_in->read_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_count())
    {
        count = (uint32_t) m.count();
        if (count < 1 || count >= (1 << 24))
        {
            FURNACE_DEBUG("read_pa: count too big: %lu\n", count);
            st = VMI_FAILURE;
        }
        res = (char*) malloc(count);
        if (res == NULL)
        {
            FURNACE_DEBUG("read_pa: malloc failed\n");
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("read_pa: missing count\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_pa(vmi, paddr, count, res, &bytes_read);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_pa: count=%lu, bytes_read=%lu, paddr=%lu\n", count, bytes_read, paddr);
        /*
        if (DEBUG) {
          printf("data glimpse\n");
          unsigned long span = bytes_read;
          if (bytes_read > 64) {
            span = 64;
          }
          for (unsigned long i=0; i<span; i++) {
            printf("%02X ", res[i]);
          }
          //printf("...\n");
        }
        */
        FURNACE_DEBUG("read_pa: loading protobuf\n");
        ret->set_result((const char*) res, bytes_read);
        ret->set_bytes_read(bytes_read);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_PA_RET);
}

void
furnace::process_read_va(int index,
                         FurnaceProto::VMIMessage* msg_in,
                         FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* res = NULL;
    status_t st = VMI_SUCCESS;
    size_t count = 0;
    int pid = 0;
    size_t bytes_read = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_va(index);
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
        FURNACE_DEBUG("read_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_count())
    {
        count = (uint32_t) m.count();
        if (count < 1 || count >= (1 << 24))
        {
            FURNACE_DEBUG("read_va: count too big: %lu\n", count);
            st = VMI_FAILURE;
        }
        res = (char*) malloc(count);
        if (res == NULL)
        {
            FURNACE_DEBUG("read_va: malloc failed\n");
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("read_va: missing count\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_va(vmi, vaddr, pid, count, res, &bytes_read);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_va: c=%lu, vaddr=%lu, pid=%d, st=%d, b_read=%lu\n", count, vaddr, pid, st, bytes_read);
        /*
        if (DEBUG && count > 16) {
          printf("data glimpse\n");
          for (int i=0; i<16; i++) {
            printf("%02X ", res[i]);
          }
          printf("...\n");
        }
        */
        ret->set_result((const char*) res, bytes_read);
        ret->set_bytes_read(bytes_read);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_VA_RET);
}

void
furnace::process_read_ksym(int index,
                           FurnaceProto::VMIMessage* msg_in,
                           FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* res = NULL;
    status_t st = VMI_SUCCESS;
    size_t count = 0;
    size_t bytes_read = 0;
    std::string symbol;
    auto m = msg_in->read_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    if (m.has_count())
    {
        count = (uint32_t) m.count();
        if (count < 1 || count >= (1 << 24))
        {
            FURNACE_DEBUG("read_ksym: count too big: %lu\n", count);
            st = VMI_FAILURE;
        }
        res = (char*) malloc(count);
        if (res == NULL)
        {
            FURNACE_DEBUG("read_ksym: malloc failed\n");
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("read_ksym: missing count\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_ksym(vmi, (const char*) symbol.c_str(), count, res, &bytes_read);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_ksym: count=%lu, symbol=%s\n", count, symbol.c_str());
        if (DEBUG && count > 16)
        {
            printf("data glimpse\n");
            for (int i=0; i<16; i++)
            {
                printf("%02X ", res[i]);
            }
            printf("...\n");
        }
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_KSYM_RET);
}

void
furnace::process_read_8_pa(int index,
                           FurnaceProto::VMIMessage* msg_in,
                           FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint8_t res = 0;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->read_8_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_8_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_8_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_8_pa(vmi, paddr, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_8_pa: paddr=%lu\n", paddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_PA_RET);
}

void
furnace::process_read_16_pa(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint16_t res = 0;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->read_16_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_16_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_16_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_16_pa(vmi, paddr, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_16_pa: paddr=%lu\n", paddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_PA_RET);
}

void
furnace::process_read_32_pa(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint32_t res = 0;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->read_32_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_32_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_32_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_32_pa(vmi, paddr, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_32_pa: paddr=%lu\n", paddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_PA_RET);
    return;
}

void
furnace::process_read_64_pa(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->read_64_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_64_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_64_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_64_pa(vmi, paddr, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_64_pa: paddr=%lu\n", paddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_PA_RET);
    return;
}

void
furnace::process_read_addr_pa(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    addr_t res = 0;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->read_addr_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_addr_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_addr_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_addr_pa(vmi, paddr, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_addr_pa: paddr=%lu\n", paddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_ADDR_PA_RET);
    return;
}

void
furnace::process_read_str_pa(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* res = NULL;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->read_str_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("read_str_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_str_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        res = vmi_read_str_pa(vmi, paddr);
        drakvuf_release_vmi(this->drakvuf);
        if (res != NULL)
        {
            st = VMI_SUCCESS;
            ret->set_result(res);
            //free(res);
        }
        else
        {
            st = VMI_FAILURE;
            FURNACE_DEBUG("read_str_pa: res == NULL\n");
        }
        FURNACE_DEBUG("read_str_pa: paddr=%lu\n", paddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_STR_PA_RET);
    return;
}

void
furnace::process_read_8_va(int index,
                           FurnaceProto::VMIMessage* msg_in,
                           FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
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
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("read_8_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        st = vmi_read_8_va(vmi, vaddr, pid, &res);  // locking up??
        FURNACE_DEBUG("read_8_va: st=%d, res=%d\n", st, res);
        drakvuf_release_vmi(this->drakvuf);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_VA_RET);
    return;
}

void
furnace::process_read_16_va(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
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
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_16_va(vmi, vaddr, pid, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_16_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_VA_RET);
    return;
}

void
furnace::process_read_32_va(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
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
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("read_32_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        st = vmi_read_32_va(vmi, vaddr, pid, &res);
        FURNACE_DEBUG("read_32_va: st=%d, res=%u\n", st, res);
        drakvuf_release_vmi(this->drakvuf);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_VA_RET);
    return;
}

void
furnace::process_read_64_va(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
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
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_64_va(vmi, vaddr, pid, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_64_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_VA_RET);
    return;
}

void
furnace::process_read_addr_va(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    addr_t res = 0;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_addr_va(index);
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
        FURNACE_DEBUG("read_addr_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_addr_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_addr_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_addr_va(vmi, vaddr, pid, &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_addr_va: pid=%d, vaddr=%lu -> %lu\n", pid, vaddr, res);
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_ADDR_VA_RET);
    return;
}

void
furnace::process_read_str_va(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* res = NULL;
    status_t st = VMI_SUCCESS;
    int pid = 0;
    uint64_t vaddr = 0;
    auto m = msg_in->read_str_va(index);
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
        FURNACE_DEBUG("read_str_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("read_str_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_str_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("read_str_va: pid=%d, vaddr=%lu\n", pid, vaddr);
        res = vmi_read_str_va(vmi, vaddr, pid);
        FURNACE_DEBUG("read_str_va: res=%s\n", res);
        drakvuf_release_vmi(this->drakvuf);
        if (res != NULL)
        {
            st = VMI_SUCCESS;
            ret->set_result(res);
            //free(res);
        }
        else
        {
            st = VMI_FAILURE;
            FURNACE_DEBUG("read_str_va: res == NULL\n");
        }
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_STR_VA_RET);
    return;
}

void
furnace::process_read_8_ksym(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint8_t res = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->read_8_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_8_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_8_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_8_ksym(vmi, (char*) symbol.c_str(), &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_8_ksym\n");
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_KSYM_RET);
    return;
}

void
furnace::process_read_16_ksym(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint16_t res = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->read_16_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_16_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_16_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_16_ksym(vmi, (char*) symbol.c_str(), &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_16_ksym\n");
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_KSYM_RET);
    return;
}

void
furnace::process_read_32_ksym(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint32_t res = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->read_32_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_32_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_32_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_32_ksym(vmi, (char*) symbol.c_str(), &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_32_ksym\n");
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_KSYM_RET);
    return;
}

void
furnace::process_read_64_ksym(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->read_64_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_32_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_64_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_64_ksym(vmi, (char*) symbol.c_str(), &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_64_ksym\n");
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_KSYM_RET);
    return;
}

void
furnace::process_read_addr_ksym(int index,
                                FurnaceProto::VMIMessage* msg_in,
                                FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t res = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->read_addr_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_addr_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_addr_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_read_addr_ksym(vmi, (char*) symbol.c_str(), &res);
        drakvuf_release_vmi(this->drakvuf);
        FURNACE_DEBUG("read_addr_ksym\n");
        ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_ADDR_KSYM_RET);
    return;
}

void
furnace::process_read_str_ksym(int index,
                               FurnaceProto::VMIMessage* msg_in,
                               FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* res = NULL;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->read_str_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("read_str_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_read_str_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        res = vmi_read_str_ksym(vmi, (char*) symbol.c_str());
        drakvuf_release_vmi(this->drakvuf);
        if (res != NULL)
        {
            st = VMI_SUCCESS;
            ret->set_result(res);
            //free(res);
        }
        else
        {
            st = VMI_FAILURE;
            FURNACE_DEBUG("read_str_ksym: res == NULL\n");
        }
        //FURNACE_DEBUG("read_str_ksym\n");
        //ret->set_result(res);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_STR_KSYM_RET);
    return;
}

void
furnace::process_write_pa(int index,
                          FurnaceProto::VMIMessage* msg_in,
                          FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    size_t bytes_written = 0;
    size_t count = 0;
    status_t st = VMI_SUCCESS;
    uint64_t paddr = 0;
    auto m = msg_in->write_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("write_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_count())
    {
        count = (uint32_t) m.count();
        if (count < 1 || count >= (1 << 24))
        {
            FURNACE_DEBUG("write_pa: count too big: %lu\n", count);
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("write_pa: missing count\n");
        st = VMI_FAILURE;
    }
    if (m.has_buffer())
    {
        if (m.buffer().length() != count)
        {
            FURNACE_DEBUG("write_pa: buffer size mismatch (c=%lu, blen=%lu)\n", count, m.buffer().length());
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("write_pa: missing buffer\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_pa: paddr=%lu, count=%lu\n", paddr, count);
        st = vmi_write_pa(vmi, paddr, (size_t)count, (void*)m.buffer().c_str(), &bytes_written);
        FURNACE_DEBUG("write_pa: bytes_written=%lu\n", bytes_written);
        drakvuf_release_vmi(this->drakvuf);
        ret->set_bytes_written(bytes_written);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_PA_RET);
    return;
}

void
furnace::process_write_va(int index,
                          FurnaceProto::VMIMessage* msg_in,
                          FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    size_t bytes_written = 0;
    size_t count = 0;
    status_t st = VMI_SUCCESS;
    uint64_t vaddr = 0;
    int pid = 0;
    auto m = msg_in->write_va(index);
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
        FURNACE_DEBUG("write_8_pa: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("write_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_count())
    {
        count = (uint32_t) m.count();
        if (count < 1 || count >= (1 << 24))
        {
            FURNACE_DEBUG("write_va: count too big: %lu\n", count);
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("write_va: missing count\n");
        st = VMI_FAILURE;
    }
    if (m.has_buffer())
    {
        if (m.buffer().length() != count)
        {
            FURNACE_DEBUG("write_va: buffer size mismatch (c=%lu, blen=%lu)\n", count, m.buffer().length());
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("write_va: missing buffer\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_va: vaddr=%lu, count=%lu\n", vaddr, count);
        st = vmi_write_va(vmi, vaddr, pid, (size_t)count, (void*)m.buffer().c_str(), &bytes_written);
        FURNACE_DEBUG("write_va: bytes_written=%lu\n", bytes_written);
        drakvuf_release_vmi(this->drakvuf);
        ret->set_bytes_written(bytes_written);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_VA_RET);
    return;
}

void
furnace::process_write_ksym(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    size_t bytes_written = 0;
    size_t count = 0;
    status_t st = VMI_SUCCESS;
    std::string symbol;
    auto m = msg_in->write_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("write_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    if (m.has_count())
    {
        count = (uint32_t) m.count();
        if (count < 1 || count >= (1 << 24))
        {
            FURNACE_DEBUG("write_ksym: count too big: %lu\n", count);
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("write_ksym: missing count\n");
        st = VMI_FAILURE;
    }
    if (m.has_buffer())
    {
        if (m.buffer().length() != count)
        {
            FURNACE_DEBUG("write_ksym: buffer size mismatch (c=%lu, blen=%lu)\n", count, m.buffer().length());
            st = VMI_FAILURE;
        }
    }
    else
    {
        FURNACE_DEBUG("write_ksym: missing buffer\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_ksym: symbol=%s, count=%lu\n", symbol.c_str(), count);
        st = vmi_write_ksym(vmi, (char*)symbol.c_str(), count, (void*)m.buffer().c_str(), &bytes_written);
        FURNACE_DEBUG("write_ksym: bytes_written=%lu\n", bytes_written);
        drakvuf_release_vmi(this->drakvuf);
        ret->set_bytes_written(bytes_written);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_KSYM_RET);
    return;
}

void
furnace::process_write_8_pa(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint8_t value = 0;
    uint64_t paddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_8_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("write_8_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint8_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_8_pa: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_8_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_8_pa: paddr=%lu, value=%d\n", paddr, value);
        st = vmi_write_8_pa(vmi, paddr, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_8_PA_RET);
    return;
}

void
furnace::process_write_16_pa(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint16_t value = 0;
    uint64_t paddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_16_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("write_16_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint16_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_16_pa: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_16_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_16_pa: paddr=%lu, value=%d\n", paddr, value);
        st = vmi_write_16_pa(vmi, paddr, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_16_PA_RET);
    return;
}

void
furnace::process_write_32_pa(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint32_t value = 0;
    uint64_t paddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_32_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("write_32_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint32_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_32_pa: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_32_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_32_pa: paddr=%lu, value=%d\n", paddr, value);
        st = vmi_write_32_pa(vmi, paddr, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_32_PA_RET);
    return;
}

void
furnace::process_write_64_pa(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t value = 0;
    uint64_t paddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_64_pa(index);
    if (m.has_link() && !process_link(&m, &(m.link()), index, msg_in, msg_out))
    {
        batch_error(index, msg_out);
        return;
    }
    if (m.has_paddr())
    {
        paddr = m.paddr();
    }
    else
    {
        FURNACE_DEBUG("write_64_pa: missing paddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint64_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_64_pa: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_64_pa_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_64_pa: paddr=%lu, value=%lu\n", paddr, value);
        st = vmi_write_64_pa(vmi, paddr, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_64_PA_RET);
    return;
}

void
furnace::process_write_8_va(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint8_t value = 0;
    int pid = 0;
    uint64_t vaddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_8_va(index);
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
        FURNACE_DEBUG("write_8_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("write_8_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint8_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_8_va: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_8_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_8_va: pid=%d, vaddr=%lu, value=%d\n", pid, vaddr, value);
        st = vmi_write_8_va(vmi, vaddr, pid, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_8_VA_RET);
    return;
}

void
furnace::process_write_16_va(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint16_t value = 0;
    int pid = 0;
    uint64_t vaddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_16_va(index);
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
        FURNACE_DEBUG("write_16_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("write_16_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint16_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_16_va: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_16_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_16_va: pid=%d, vaddr=%lu, value=%d\n", pid, vaddr, value);
        st = vmi_write_16_va(vmi, vaddr, pid, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_16_VA_RET);
    return;
}

void
furnace::process_write_32_va(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint32_t value = 0;
    int pid = 0;
    uint64_t vaddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_32_va(index);
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
        FURNACE_DEBUG("write_32_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("write_32_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint32_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_32_va: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_32_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_32_va: pid=%d, vaddr=%lu, value=%d\n", pid, vaddr, value);
        st = vmi_write_32_va(vmi, vaddr, pid, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_32_VA_RET);
    return;
}

void
furnace::process_write_64_va(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t value = 0;
    int pid = 0;
    uint64_t vaddr = 0;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_64_va(index);
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
        FURNACE_DEBUG("write_64_va: missing PID\n");
        st = VMI_FAILURE;
    }
    if (m.has_vaddr())
    {
        vaddr = m.vaddr();
    }
    else
    {
        FURNACE_DEBUG("write_64_va: missing vaddr\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint64_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_64_va: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_64_va_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_64_va: pid=%d, vaddr=%lu, value=%lu\n", pid, vaddr, value);
        st = vmi_write_64_va(vmi, vaddr, pid, &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_64_VA_RET);
    return;
}

void
furnace::process_write_8_ksym(int index,
                              FurnaceProto::VMIMessage* msg_in,
                              FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint8_t value = 0;
    std::string symbol;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_8_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("write_8_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint8_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_8_ksym: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_8_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_8_ksym: symbol=%s, value=%d\n", symbol.c_str(), value);
        st = vmi_write_8_ksym(vmi, (char*)symbol.c_str(), &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_8_KSYM_RET);
    return;
}

void
furnace::process_write_16_ksym(int index,
                               FurnaceProto::VMIMessage* msg_in,
                               FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint16_t value = 0;
    std::string symbol;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_16_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("write_16_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint16_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_16_ksym: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_16_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_16_ksym: symbol=%s, value=%d\n", symbol.c_str(), value);
        st = vmi_write_16_ksym(vmi, (char*)symbol.c_str(), &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_16_KSYM_RET);
    return;
}

void
furnace::process_write_32_ksym(int index,
                               FurnaceProto::VMIMessage* msg_in,
                               FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint32_t value = 0;
    std::string symbol;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_32_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("write_32_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint32_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_32_ksym: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_32_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_32_ksym: symbol=%s, value=%d\n", symbol.c_str(), value);
        st = vmi_write_32_ksym(vmi, (char*)symbol.c_str(), &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_32_KSYM_RET);
    return;
}

void
furnace::process_write_64_ksym(int index,
                               FurnaceProto::VMIMessage* msg_in,
                               FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    uint64_t value = 0;
    std::string symbol;
    status_t st = VMI_SUCCESS;
    auto m = msg_in->write_64_ksym(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("write_64_ksym: symbol not set\n");
        st = VMI_FAILURE;
    }
    if (m.has_value())
    {
        value = (uint64_t) m.value();
    }
    else
    {
        FURNACE_DEBUG("write_64_ksym: missing value\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_write_64_ksym_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        FURNACE_DEBUG("write_64_ksym: symbol=%s, value=%lu\n", symbol.c_str(), value);
        st = vmi_write_64_ksym(vmi, (char*)symbol.c_str(), &value);
        drakvuf_release_vmi(this->drakvuf);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_64_KSYM_RET);
    return;
}

void
furnace::process_get_name(int index,
                          FurnaceProto::VMIMessage* msg_in,
                          FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi;
    char* name = NULL;
    std::string sname;
    vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    name = vmi_get_name(vmi);
    sname = name;
    free(name);
    drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("get_name: %s\n", sname.c_str());
    msg_out->mutable_get_name_ret()->set_status(VMI_SUCCESS);
    msg_out->mutable_get_name_ret()->set_result(sname.c_str(), sname.length());
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_NAME_RET);
    return;
}

void
furnace::process_get_vmid(int index,
                          FurnaceProto::VMIMessage* msg_in,
                          FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    uint64_t vmid = vmi_get_vmid(vmi);
    drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("get_vmid: %lu\n", vmid);
    msg_out->mutable_get_vmid_ret()->set_status(VMI_SUCCESS);
    msg_out->mutable_get_vmid_ret()->set_result(vmid);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_VMID_RET);
    return;
}

// TODO
void
furnace::process_get_access_mode(int index,
                                 FurnaceProto::VMIMessage* msg_in,
                                 FurnaceProto::VMIMessage* msg_out)
{
    // Not implemented.
    this->set_batch_error(1, msg_out);
    this->batch_error(index, msg_out);
    return;
    /*
    //vmi_instance_t vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    status_t access_mode = VMI_SUCCESS; //vmi_get_access_mode(vmi, );
    //drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("get_access_mode: %u\n", access_mode);
    msg_out->mutable_get_access_mode_ret()->set_status(VMI_SUCCESS);
    msg_out->mutable_get_access_mode_ret()->set_result(access_mode);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_ACCESS_MODE_RET);
    return;
    */
}

void
furnace::process_get_winver(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    int32_t winver = vmi_get_winver(vmi);
    drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("get_winver: %d\n", winver);
    msg_out->mutable_get_winver_ret()->set_status(VMI_SUCCESS);
    msg_out->mutable_get_winver_ret()->set_result(winver);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_WINVER_RET);
    return;
}

void
furnace::process_get_winver_str(int index,
                                FurnaceProto::VMIMessage* msg_in,
                                FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    const char* winver = vmi_get_winver_str(vmi);
    drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("get_winver_str: %s\n", winver);
    msg_out->mutable_get_winver_str_ret()->set_status(VMI_SUCCESS);
    msg_out->mutable_get_winver_str_ret()->set_result(winver);
    //free(winver);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_WINVER_STR_RET);
    return;
}

void
furnace::process_get_vcpureg(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    // Not implemented.
    this->set_batch_error(1, msg_out);
    this->batch_error(index, msg_out);
    return;
}

void
furnace::process_get_memsize(int index,
                             FurnaceProto::VMIMessage* msg_in,
                             FurnaceProto::VMIMessage* msg_out)
{
    vmi_instance_t vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    uint64_t memsize = vmi_get_memsize(vmi);
    drakvuf_release_vmi(this->drakvuf);
    FURNACE_DEBUG("get_memsize: %lu\n", memsize);
    msg_out->mutable_get_memsize_ret()->set_status(VMI_SUCCESS);
    msg_out->mutable_get_memsize_ret()->set_result(memsize);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_MEMSIZE_RET);
    return;
}

void
furnace::process_get_offset(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    vmi_instance_t vmi;
    std::string symbol;
    uint64_t offset = 0;
    auto m = msg_in->get_offset(index);
    if (m.has_symbol())
    {
        symbol = m.symbol();
    }
    else
    {
        FURNACE_DEBUG("get_offset: symbol not set\n");
        st = VMI_FAILURE;
    }
    auto ret = msg_out->add_get_offset_ret();
    if (st == VMI_SUCCESS)
    {
        vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
        st = vmi_get_offset(vmi, (const char*)symbol.c_str(), &offset);
        drakvuf_release_vmi(this->drakvuf);
        ret->set_result(offset);
        FURNACE_DEBUG("get_offset: %s: %lu\n", symbol.c_str(), offset);
    }
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_OFFSET_RET);
    return;
}

void
furnace::process_get_ostype(int index,
                            FurnaceProto::VMIMessage* msg_in,
                            FurnaceProto::VMIMessage* msg_out)
{
    // Not implemented.
    this->set_batch_error(1, msg_out);
    this->batch_error(index, msg_out);
    return;
}

void
furnace::process_get_page_mode(int index,
                               FurnaceProto::VMIMessage* msg_in,
                               FurnaceProto::VMIMessage* msg_out)
{
    // Not implemented.
    this->set_batch_error(1, msg_out);
    this->batch_error(index, msg_out);
    return;
}

void furnace::process_status(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    FURNACE_DEBUG("FURNACE: Got status\n");

    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_STATUS);
    msg_out->mutable_status()->set_state(1);
    return;
}

void furnace::process_event_register(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;

    if (this->events.size() >= (unsigned long) this->max_events)
    {
        printf("Reached maximum number of events (%lu)!\n", this->events.size());
        st = VMI_FAILURE;
        this->set_batch_error(1, msg_out);
        this->batch_error(index, msg_out);
        return;
    }

    drakvuf_trap_t* furn_event = (drakvuf_trap_t*) malloc(sizeof(drakvuf_trap_t));
    event_item* ei = (event_item*) malloc(sizeof(event_item));
    if (ei == NULL || furn_event == NULL)
    {
        printf("Failed to allocate trap memory!\n");
        this->set_batch_error(2, msg_out);
        this->batch_error(index, msg_out);
        return;
    }
    memset(furn_event, 0, sizeof(drakvuf_trap_t));
    memset(ei, 0, sizeof(event_item));

    FURNACE_DEBUG("event_register switch statement (%lu/%d)\n", this->events.size(), this->max_events);
    auto m = msg_in->event_register(index);
    auto event_type = m.event_type();
    switch (event_type)
    {
        //case FurnaceProto::Event_register_EventType::Event_register_EventType_F_REG:
        case FurnaceProto::F_REG:
            FURNACE_DEBUG("event_register register event\n");
            switch (m.reg_value())
            {
                //case FurnaceProto::Event_register_RegType::Event_register_RegType_F_CR3:
                case FurnaceProto::F_CR3:
                    furn_event->reg = CR3;
                    break;
                default:
                    st = VMI_FAILURE;
                    this->set_batch_error(3, msg_out);
                    this->batch_error(index, msg_out);
                    printf("Register type NOT implemented!\n");
                    break;
            }
            furn_event->type = REGISTER;
            furn_event->cb = &furnace_cr3_callback;
            furn_event->name = "Furnace_CR3";
            break;
        //case FurnaceProto::Event_register_EventType::Event_register_EventType_F_INT:
        case FurnaceProto::F_INT:
            FURNACE_DEBUG("event_register interrupt event\n");
            furn_event->type = BREAKPOINT;
            furn_event->breakpoint.lookup_type = LOOKUP_PID;
            furn_event->breakpoint.addr_type = ADDR_VA;
            furn_event->breakpoint.addr = m.bp_addr();
            furn_event->breakpoint.pid = m.bp_pid();
            furn_event->name = strdup(m.bp_name().c_str());
            furn_event->cb = &furnace_int_callback;
            break;
        case FurnaceProto::F_EPT:
            //case FurnaceProto::Event_register_EventType::Event_register_EventType_F_EPT:
            FURNACE_DEBUG("event_register mem event\n");
        //TODO
        /*
        furn_event->type = MEMACCESS;
        furn_event->memaccess.gfn = 0;  // TODO
        furn_event->memaccess.type = PRE;  // TODO
        furn_event->memaccess.access = VMI_MEMACCESS_W;  // TODO
        furn_event->name = strdup(m.bp_name().c_str());
        furn_event->cb = &furnace_ept_callback;
        break;
        */
        default:
            printf("Event type NOT implemented!\n");
            this->set_batch_error(4, msg_out);
            this->batch_error(index, msg_out);
            free(furn_event);
            free(ei);
            st = VMI_FAILURE;
            return;
            break;
    }

    auto ret = msg_out->add_event_register_ret();
    if (st == VMI_SUCCESS)
    {
        ei->id = std::numeric_limits<uint64_t>::max();  // the actual id is set inside add_event
        ei->sync_state = m.sync_state();
        ei->active = true;
        ei->furn = this;
        ei->event = furn_event;
        ei->owner = strdup(this->scratch_ident);
        ei->owner[strlen(ei->owner)-1] = 'd';
        furn_event->data = (void*) ei;
        //ei->fe_source = "";

        FURNACE_DEBUG("event_register add_event statement, owner=%s\n", ei->owner);
        add_event(ei, &this->events);
        FURNACE_DEBUG("ident=%lu, map has %lu entries\n", ei->id, this->events.size());

        //drakvuf_lock_and_get_vmi(this->drakvuf);
        drakvuf_pause(this->drakvuf);
        //st = VMI_SUCCESS;
        if (drakvuf_add_trap(this->drakvuf, furn_event))
        {
            ret->set_identifier(ei->id);
        }
        else
        {
            printf("ERROR with drakvuf_add_trap!\n");
            this->set_batch_error(5, msg_out);
            this->batch_error(index, msg_out);
            ret->set_identifier(0);
            st = VMI_FAILURE;
            this->events.erase(ei->id);
            free(furn_event);
            free(ei);
        }
        drakvuf_resume(this->drakvuf);
        //drakvuf_release_vmi(this->drakvuf);
    }

    FURNACE_DEBUG("event_register: status:%d, identifier:%lu\n", st, ret->identifier());

    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT_REGISTER_RET);

    return;
}

void furnace::process_event_clear(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    auto m = msg_in->event_clear(index);
    uint64_t id = m.identifier();
    status_t st = VMI_SUCCESS;
    event_item* ei = NULL;
    FURNACE_DEBUG("DOING event_clear: id=%lu\n", id);

    //drakvuf_lock_and_get_vmi(this->drakvuf);
    drakvuf_pause(this->drakvuf);

    FURNACE_DEBUG("event_clear tracker\n");
    if (is_event_present(id, &this->events))
    {
        FURNACE_DEBUG("Found %lu in events\n", id);
        this->it = this->events.find(id);
        ei = this->it->second;
        FURNACE_DEBUG("removing trap\n");
        drakvuf_remove_trap(this->drakvuf, ei->event, (drakvuf_trap_free_t)free);
        FURNACE_DEBUG("removing from event map\n");
        this->events.erase(ei->id);
        FURNACE_DEBUG("freeing owner\n");
        free(ei->owner);
        FURNACE_DEBUG("freeing event_item\n");
        free(ei);
        FURNACE_DEBUG("After deletion, map has %lu entries\n", events.size());
    }
    else
    {
        FURNACE_DEBUG("Event %lu not found\n", id);
        this->set_batch_error(1, msg_out);
        this->batch_error(index, msg_out);
        st = VMI_FAILURE;
    }

    drakvuf_resume(this->drakvuf);
    //drakvuf_release_vmi(this->drakvuf);

    FURNACE_DEBUG("event_clear: status:%d, identifier:%lu\n", st, id);

    auto ret = msg_out->add_event_clear_ret();
    ret->set_status(st);
    ret->set_identifier(id);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT_CLEAR_RET);

    return;
}

void furnace::process_events_start(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    FURNACE_DEBUG("events_start\n");
    this->events_active = true;

    msg_out->mutable_events_start_ret()->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENTS_START_RET);
    return;
}

void furnace::process_events_stop(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    FURNACE_DEBUG("events_stop\n");
    this->events_active = false;

    msg_out->mutable_events_stop_ret()->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENTS_STOP_RET);
    return;
}

void furnace::process_resume_vm(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    FURNACE_DEBUG("resume_vm\n");
    drakvuf_lock_and_get_vmi(this->drakvuf);
    drakvuf_resume(this->drakvuf);
    drakvuf_release_vmi(this->drakvuf);

    auto ret = msg_out->add_resume_vm_ret();
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_RESUME_VM_RET);
    return;
}

void furnace::process_pause_vm(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    FURNACE_DEBUG("pause_vm\n");
    drakvuf_lock_and_get_vmi(this->drakvuf);
    drakvuf_pause(this->drakvuf);
    drakvuf_release_vmi(this->drakvuf);

    auto ret = msg_out->add_pause_vm_ret();
    ret->set_status(st);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_PAUSE_VM_RET);
    return;
}

void furnace::process_lookup_symbol(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    uint64_t res = 0;
    auto m = msg_in->lookup_symbol(index);
    std::string sym = m.symbol_name();
    FURNACE_DEBUG("lookup_symbol\n");

    if (!drakvuf_get_constant_rva(this->rekall_profile.c_str(),
                                  sym.c_str(), &res))
    {
        FURNACE_DEBUG("lookup_symbol failed\n");
        st = VMI_FAILURE;
    }

    auto ret = msg_out->add_lookup_symbol_ret();
    ret->set_status(st);
    ret->set_result(res);
    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_LOOKUP_SYMBOL_RET);
    return;
}

void furnace::process_lookup_structure(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out)
{
    status_t st = VMI_SUCCESS;
    uint64_t res = 0;
    auto m = msg_in->lookup_structure(index);
    std::string structure = m.structure_name();
    std::string key = m.structure_key();
    FURNACE_DEBUG("lookup_structure\n");

    if (!drakvuf_get_struct_member_rva(this->rekall_profile.c_str(),
                                       structure.c_str(), key.c_str(), &res))
    {
        FURNACE_DEBUG("lookup_structure failed\n");
        st = VMI_FAILURE;
    }

    auto ret = msg_out->add_lookup_structure_ret();
    ret->set_status(st);
    ret->set_result(res);

    msg_out->add_type(FurnaceProto::VMIMessage_Type::VMIMessage_Type_LOOKUP_STRUCTURE_RET);
    return;
}


void
furnace::process_single(FurnaceProto::VMIMessage_Type msg_type,
                        int index,
                        FurnaceProto::VMIMessage* msg_in,
                        FurnaceProto::VMIMessage* msg_out)
{

    this->call_ctr++;
    FURNACE_DEBUG("process_single: t=%d, i=%d, cc=%lu\n", msg_type, index, this->call_ctr);
    switch (msg_type)
    {
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_TRANSLATE_KV2P:
            process_translate_kv2p(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_TRANSLATE_UV2P:
            process_translate_uv2p(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_TRANSLATE_KSYM2V:
            process_translate_ksym2v(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_PA:
            process_read_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_VA:
            process_read_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_KSYM:
            process_read_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_PA:
            process_read_8_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_PA:
            process_read_16_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_PA:
            process_read_32_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_PA:
            process_read_64_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_ADDR_PA:
            process_read_addr_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_STR_PA:
            process_read_str_pa(index, msg_in, msg_out);
            break;
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
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_ADDR_VA:
            process_read_addr_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_STR_VA:
            process_read_str_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_8_KSYM:
            process_read_8_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_16_KSYM:
            process_read_16_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_32_KSYM:
            process_read_32_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_64_KSYM:
            process_read_64_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_ADDR_KSYM:
            process_read_addr_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_READ_STR_KSYM:
            process_read_str_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_PA:
            process_write_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_VA:
            process_write_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_KSYM:
            process_write_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_8_PA:
            process_write_8_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_16_PA:
            process_write_16_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_32_PA:
            process_write_32_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_64_PA:
            process_write_64_pa(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_8_VA:
            process_write_8_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_16_VA:
            process_write_16_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_32_VA:
            process_write_32_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_64_VA:
            process_write_64_va(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_8_KSYM:
            process_write_8_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_16_KSYM:
            process_write_16_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_32_KSYM:
            process_write_32_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_WRITE_64_KSYM:
            process_write_64_ksym(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_NAME:
            process_get_name(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_VMID:
            process_get_vmid(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_ACCESS_MODE:
            process_get_access_mode(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_WINVER:
            process_get_winver(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_WINVER_STR:
            process_get_winver_str(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_VCPUREG:
            process_get_vcpureg(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_MEMSIZE:
            process_get_memsize(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_OFFSET:
            process_get_offset(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_OSTYPE:
            process_get_ostype(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_GET_PAGE_MODE:
            process_get_page_mode(index, msg_in, msg_out);
            break;

        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_STATUS:
            process_status(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT_REGISTER:
            process_event_register(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENT_CLEAR:
            process_event_clear(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENTS_START:
            process_events_start(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_EVENTS_STOP:
            process_events_stop(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_RESUME_VM:
            process_resume_vm(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_PAUSE_VM:
            process_pause_vm(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_LOOKUP_SYMBOL:
            process_lookup_symbol(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_LOOKUP_STRUCTURE:
            process_lookup_structure(index, msg_in, msg_out);
            break;

        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_PROCESS_LIST:
            process_process_list(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_START_SYSCALL:
            process_start_syscall(index, msg_in, msg_out);
            break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_STOP_SYSCALL:
            process_stop_syscall(index, msg_in, msg_out);
            break;
        /*
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_START_VMIFS:
          process_start_vmifs(index, msg_in, msg_out); break;
        case FurnaceProto::VMIMessage_Type::VMIMessage_Type_STOP_VMIFS:
          process_stop_vmifs(index, msg_in, msg_out); break;
          */

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
furnace::process_protobuf(char* buf, int len)
{

    FurnaceProto::VMIMessage_Type msg_type;
    std::map<FurnaceProto::VMIMessage_Type, int> typecount;
    std::string r;
    //const std::string sbuf = buf;

    FURNACE_PROFILE("PBINIT");
    if (this->msg_in.ParseFromArray(buf, len) == false)
    {
        printf("FURNACE: could not parse incoming protobuf\n");
    }
    this->msg_out.Clear();
    this->cache.clear();  // clear the link cache from the prior message
    this->nth_type = 0;
    //this->nth_field = 0;
    this->nth_field_ix = 0;
    this->msgno = 0;
    FURNACE_DEBUG("FURNACE: message (%dB) has %d types\n", len, this->msg_in.type_size());

    if (this->msg_in.type_size() > 1000)
    {
        FURNACE_DEBUG("FURNACE: Error, too many messages (%d)\n", this->msg_in.type_size());

    }
    else
    {

        for (int i=0; i<this->msg_in.type_size(); i++)
        {
            msg_type = this->msg_in.type(i);
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
            process_single(msg_type, typecount[msg_type], &(this->msg_in), &(this->msg_out));
            this->msgno++;
            FURNACE_PROFILE("FNSIGL");
        }
    }

    this->msg_out.SerializeToString(&r);
    FURNACE_PROFILE("PBSERL");
    FURNACE_DEBUG("FURNACE: sending message (%luB)\n", r.length());
    return r;
}

int furnace::process_zmq(void* zsock)
{
    uint32_t zevents = 0;
    size_t zevents_s = sizeof(zevents);
    int rc;
    char* ident;

    do
    {
        rc = zmq_getsockopt(zsock, ZMQ_EVENTS, &zevents, &zevents_s);
        assert(rc == 0);
        if (zevents & ZMQ_POLLIN)
        {

            zmq_msg_t zmsg;
            rc = zmq_msg_init(&zmsg);

            int part = 1;
            do
            {
                rc = zmq_msg_recv(&zmsg, zsock, ZMQ_DONTWAIT);

                if (part != 2)
                {
                    FURNACE_DEBUG("FURNACE: part != 2 message part %d of size %dB\n", part, rc);
                    assert(rc > 0); // can be == 0 because of null message part
                }

                FURNACE_DEBUG("FURNACE: received message part %d of size %dB\n", part, rc);
                char* buf = (char*) zmq_msg_data(&zmsg);
                std::string r;

                switch (part)
                {
                    case 1:
                        ident = strndup(buf, rc);
                        this->scratch_ident = ident;
                        FURNACE_DEBUG("identity size=%d: %s\n", rc, ident);
                        break;
                    case 2:
                        break;
                    case 3:
                        this->msg_ctr++;
                        r = process_protobuf(buf, rc);
                        zmq_msend(zsock, ident, r);
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


int furnace::do_uid_chown(const char* fp, uid_t u, gid_t g)
{
    int r = 0;
    if (chown(fp, u, g) == -1)
    {
        printf("chown call failed\n");
        r = 1;
    }
    return r;
}
int furnace::do_chown(const char* fp, const char* un, const char* gn)
{
    int r = 0;
    struct passwd* pwd = getpwnam(un);
    if (pwd == NULL)
    {
        printf("Failed to get uid\n");
        r = 1;
    }
    uid_t uid = pwd->pw_uid;

    struct group* grp = getgrnam(gn);
    if (grp == NULL)
    {
        printf("Failed to get gid\n");
        r = 1;
    }
    gid_t gid = grp->gr_gid;

    r = furnace::do_uid_chown(fp, uid, gid);
    return r;
}


void
furnace::add_event(event_item* e, std::map<uint64_t, event_item*>* el)
{
    int i = 0;
    while (furnace::is_event_present(i, el))
    {
        i++;
    }
    e->id = i;
    (*el)[i] = e;
    FURNACE_DEBUG("added event to id slot %lu!\n", e->id);
    return;
}

bool
furnace::is_event_present(uint64_t id, std::map<uint64_t, event_item*>* el)
{
    return !(el->find(id) == el->end());
}

/* ----------------------------------------------------- */

furnace::furnace(drakvuf_t drakvuf, const void* config, output_format_t output)
{
    this->rekall_profile = (const char*)config;
    this->drakvuf = drakvuf;
    this->events_active = false;
    this->msg_ctr = 0;
    this->call_ctr = 0;
    this->async_send_ctr = 0;
    this->syscall_plugin = NULL;
    this->syscall_plugin_owner = NULL;

    int rc = 0;
    PRINT_DEBUG("FURNACE constructor\n");
    PRINT_DEBUG("- rekall profile: %s!\n", this->rekall_profile.c_str());

    vmi_instance_t vmi = drakvuf_lock_and_get_vmi(this->drakvuf);
    this->vm_name = (const char*) vmi_get_name(vmi);
    drakvuf_release_vmi(this->drakvuf);

    //this->pub_path = "/tmp/vmi_pub_" + this->vm_name;
    //this->rtr_path = "/opt/furnace/vmi_socket/vmi_rtr_" + this->vm_name;
    this->rtr_path = "/opt/furnace/vmi_socket/vmi_rtr";
    //this->rtr_path = "/tmp/vmi_rtr_" + this->vm_name;

    /*
    this->vmifs_mount_path = "/opt/furnace/vmifs";
    this->vmifs_bin_path = "/opt/furnace/drakvuf/libvmi/tools/vmifs/vmifs2";
    this->vmifs_pid = -1;
    //this->vmifs_mount_path = "/opt/furnace/f_vmifs";
    //this->vmifs_bin_path = "/home/micah/drakvuf/libvmi/tools/vmifs/vmifs2";
    */

    //PRINT_DEBUG("- VM: %s\n- %s\n- %s\n",
    PRINT_DEBUG("- VM: %s\n- %s\n", this->vm_name.c_str(), this->rtr_path.c_str());
    //std::string pub_uri = "ipc://" + this->pub_path;
    std::string rtr_uri = "ipc://" + this->rtr_path;

    // zmq start
    PRINT_DEBUG("- starting zmq\n");
    PRINT_DEBUG("- binding to %s\n", rtr_uri.c_str());
    void* context = zmq_ctx_new();
    zmq_ctx_set(context, ZMQ_IO_THREADS, 6);

    void* zsock_rtr = zmq_socket(context, ZMQ_ROUTER);
    rc = zmq_bind(zsock_rtr, rtr_uri.c_str());
    assert(rc == 0);

    //PRINT_DEBUG("- chowning %s\n", rtr_uri.c_str());
    //rc = do_chown(this->rtr_path.c_str(), "micah", "micah");
    //rc = do_uid_chown(this->rtr_path.c_str(), 1000, 1000);
    //assert(rc == 0);

    PRINT_DEBUG("- chmoding %s\n", rtr_uri.c_str());
    char mode[] = "0777";
    int i = strtol(mode, 0, 8);
    rc = chmod(this->rtr_path.c_str(), i);
    assert(rc == 0);

    /*
    void *zsock_pub = zmq_socket(context, ZMQ_PUB);
    rc = zmq_bind(zsock_pub, pub_uri.c_str());
    assert (rc == 0);
    PRINT_DEBUG("- chowning %s\n", pub_uri.c_str());
    rc = do_chown(this->pub_path.c_str(), "micah", "micah");
    assert(rc == 0);
    */

    PRINT_DEBUG("- getsockopt up\n");
    int fd_1;
    size_t fd_len = sizeof(fd_1);
    rc = zmq_getsockopt(zsock_rtr, ZMQ_FD, &fd_1, &fd_len);
    assert(rc == 0);
    assert(fd_len > 0);

    PRINT_DEBUG("- priming zmq fd\n");
    int timeout = 0;
    size_t timeout_len = sizeof(timeout);
    rc = zmq_setsockopt(zsock_rtr, ZMQ_RCVTIMEO, &timeout, timeout_len);
    assert(rc == 0);

    char tmp [10];
    rc = zmq_recv(zsock_rtr, tmp, 10, 0);
    assert(rc == -1);
    assert(errno & EAGAIN);

    PRINT_DEBUG("- adding to pfds, drakvuf\n");
    drakvuf_event_fd_add(drakvuf, fd_1, furnace_zmq_cb, (void*)this);
    //drakvuf_set_zmq_context(drakvuf, context);
    //drakvuf_set_zmq_socket(drakvuf, zsock_rtr);
    //drakvuf_set_furnace_obj(drakvuf, (void *) this);
    //drakvuf_set_zmq_callback(drakvuf, &zmq_hop);
    //drakvuf_set_fd(drakvuf, 0, fd_1);
    //this->zsock_pub = zsock_pub;
    this->zsock_rtr = zsock_rtr;
    this->zcontext = context;

    // signal to ptracer to strengthen the policy
    PRINT_DEBUG("- signalling to ptracer\n");
    FILE* fp = fopen("ESTABLISHED", "r");
    if (fp)
    {
        PRINT_DEBUG("- file was actually opened? %d\n", fclose(fp));
    }
    // end signal

    PRINT_DEBUG("FURNACE constructor done\n");
}


furnace::~furnace()
{
    PRINT_DEBUG("FURNACE destructor\n");
    //zmq_close(this->zsock_pub);
    zmq_close(this->zsock_rtr);
    zmq_ctx_destroy(this->zcontext);
    remove(this->rtr_path.c_str());
    //remove(this->pub_path);
    PRINT_DEBUG("Call counter:        %lu\n", this->call_ctr);
    PRINT_DEBUG("Msg counter:         %lu\n", this->msg_ctr);
    PRINT_DEBUG("Async send counter:  %lu\n", this->async_send_ctr);
    PRINT_DEBUG("FURNACE destructor done\n");
}


