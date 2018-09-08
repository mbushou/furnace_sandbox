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

#ifndef MOCK_DRIVER_H
#define MOCK_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma GCC visibility push(default)

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#define VMI_INIT_DOMAINNAME (1u << 0) /**< initialize using domain name */
#define VMI_INIT_DOMAINID (1u << 1) /**< initialize using domain id */
#define VMI_INIT_EVENTS (1u << 2) /**< initialize events */
#define VMI_INIT_SHM (1u << 3) /**< initialize SHM mode */
#define VMI_INIT_XEN_EVTCHN (1u << 4) /**< use provided Xen file descriptor */

typedef enum vmi_mode
{
    VMI_XEN, /**< libvmi is monitoring a Xen VM */
    VMI_KVM, /**< libvmi is monitoring a KVM VM */
    VMI_FILE, /**< libvmi is viewing a file on disk */
} vmi_mode_t;

typedef enum vmi_config
{
    VMI_CONFIG_GLOBAL_FILE_ENTRY, /**< config in file provided */
    VMI_CONFIG_STRING,            /**< config string provided */
    VMI_CONFIG_GHASHTABLE,        /**< config GHashTable provided */
} vmi_config_t;

struct vmi_instance
{
    uint32_t init_flags;    /**< init flags (events, shm, etc.) */
};

typedef struct vmi_instance* vmi_instance_t;

typedef uint64_t addr_t;
typedef int32_t vmi_pid_t;

typedef enum status
{
    VMI_SUCCESS,  /**< return value indicating success */
    VMI_FAILURE   /**< return value indicating failure */
} status_t;

typedef enum vmi_init_error
{
    VMI_INIT_ERROR_NONE, /**< No error */
    VMI_INIT_ERROR_DRIVER_NOT_DETECTED, /**< Failed to auto-detect hypervisor */
    VMI_INIT_ERROR_DRIVER, /**< Failed to initialize hypervisor-driver */
    VMI_INIT_ERROR_VM_NOT_FOUND, /**< Failed to find the specified VM */
    VMI_INIT_ERROR_PAGING, /**< Failed to determine or initialize paging functions */
    VMI_INIT_ERROR_OS, /**< Failed to determine or initialize OS functions */
    VMI_INIT_ERROR_EVENTS, /**< Failed to initialize events */
    VMI_INIT_ERROR_SHM, /**< Failed to initialize SHM */
    VMI_INIT_ERROR_NO_CONFIG, /**< No configuration was found for OS initialization */
    VMI_INIT_ERROR_NO_CONFIG_ENTRY, /**< Configuration contained no valid entry for VM */
} vmi_init_error_t;

struct vmi_event;
typedef struct vmi_event vmi_event_t;

// Driver
//
status_t fdrive_init_complete(
    vmi_instance_t* vmi,
    void* domain,
    uint64_t init_flags,
    void* init_data,
    vmi_config_t config_mode,
    void* config,
    vmi_init_error_t* error);


status_t fdrive_read_8_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint8_t* value);
status_t fdrive_read_16_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint16_t* value);
status_t fdrive_read_32_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint32_t* value);
status_t fdrive_read_64_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint64_t* value);

status_t fdrive_read_addr_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    addr_t* value);

char* fdrive_read_str_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid);

status_t
fdrive_rekall_profile_symbol_to_rva(
    const char* rekall_profile,
    const char* symbol,
    const char* subsymbol,
    addr_t* rva);

status_t fdrive_events_listen(
    vmi_instance_t vmi,
    uint32_t timeout);

status_t fdrive_destroy(
    vmi_instance_t vmi);

#pragma GCC visibility pop

#ifdef __cplusplus
}
#endif

#endif /* LIBVMI_H */
