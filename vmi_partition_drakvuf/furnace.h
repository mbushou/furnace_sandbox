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

#ifndef FURNACE_H
#define FURNACE_H

#include "plugins/private.h"
#include "plugins/plugins.h"
#include <string>
#include "vmi.pb.h"

class furnace;
class event_item
{
public:
    uint64_t id;
    int sync_state;
    bool active;
    std::string fe_source;
    drakvuf_trap_t* event;
    furnace* furn;
    char* owner;
};

class furnace: public plugin
{
public:

    furnace(drakvuf_t drakvuf, const void* config, output_format_t output);
    ~furnace();

    FurnaceProto::VMIMessage msg_in;
    FurnaceProto::VMIMessage msg_out;

    std::map<uint32_t, uint64_t> cache;  // link cache
    uint32_t nth_type;      // the type "stack," one entry per message, best speedup potential
    //uint32_t nth_field;     // the field type stack, usually 2 or 3 (little speedup)
    uint32_t nth_field_ix;  // the index within the field type, good speedup potential
    uint32_t msgno;         // the current message being processed

    uint64_t msg_ctr;
    uint64_t call_ctr;
    uint64_t async_send_ctr;

    // Policy
    int max_events = 200;

    std::string rekall_profile;
    std::string vm_name;
    std::string pub_path;
    std::string rtr_path;
    /*
    // we don't use VMIFS, it was a bad idea to try
    std::string vmifs_bin_path;
    std::string vmifs_mount_path;
    pid_t vmifs_pid;
    */

    // Specific to enabling syscall tracing via DK's syscall plugin
    char* syscall_plugin_owner;         // if non-null, syscall_plugin is active
    class plugin* syscall_plugin;
    int syscall_plugin_sync_state;
    event_item* syscall_plugin_ei;
    uint64_t syscall_plugin_event_id;

    char* scratch_ident;
    void* zsock_pub;
    void* zsock_rtr;
    void* zcontext;
    drakvuf_t drakvuf;
    std::map<uint64_t, event_item*> events;
    std::map<uint64_t, event_item*>::iterator it;
    bool events_active;

    void add_event(event_item* e, std::map<uint64_t, event_item*>* el);
    bool is_event_present(uint64_t id, std::map<uint64_t, event_item*>* el);
    int do_chown(const char* fp, const char* un, const char* gn);
    int do_uid_chown(const char* fp, uid_t u, gid_t g);
    int process_zmq(void* zsock);
    std::string process_protobuf(char* buf, int len);
    void process_single(FurnaceProto::VMIMessage_Type msg_type, int index,
                        FurnaceProto::VMIMessage* msg_in,
                        FurnaceProto::VMIMessage* msg_out);

    void batch_error(int index, FurnaceProto::VMIMessage* msg_out);
    void set_batch_error(int error_num, FurnaceProto::VMIMessage* msg_out);
    bool process_link(google::protobuf::Message* m, const FurnaceProto::Linkage* link,
                      int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);

    void process_translate_kv2p(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_translate_uv2p(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_translate_ksym2v(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_8_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_16_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_32_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_64_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_addr_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_str_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_8_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_16_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_32_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_64_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_addr_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_str_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_8_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_16_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_32_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_64_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_addr_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_read_str_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_8_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_16_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_32_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_64_pa(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_8_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_16_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_32_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_64_va(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_8_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_16_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_32_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_write_64_ksym(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_name(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_vmid(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_access_mode(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_winver(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_winver_str(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_vcpureg(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_memsize(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_offset(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_ostype(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_get_page_mode(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);

    void process_status(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_event_register(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_event_clear(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_events_start(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_events_stop(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_resume_vm(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_pause_vm(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_lookup_symbol(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_lookup_structure(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);

    void process_process_list(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_start_syscall(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    void process_stop_syscall(int index, FurnaceProto::VMIMessage* msg_in, FurnaceProto::VMIMessage* msg_out);
    /*
    void process_start_vmifs(int index, FurnaceProto::VMIMessage *msg_in, FurnaceProto::VMIMessage *msg_out);
    void process_stop_vmifs(int index, FurnaceProto::VMIMessage *msg_in, FurnaceProto::VMIMessage *msg_out);
    */

};

#endif
