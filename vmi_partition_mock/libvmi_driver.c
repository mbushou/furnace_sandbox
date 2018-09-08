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

#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include <libvmi/libvmi.h>
#include <libvmi/events.h>

#include "libvmi_driver.h"


status_t
fdrive_init_complete(
    vmi_instance_t* vmi,
    void* domain,
    uint64_t init_flags,
    void* init_data,
    vmi_config_t config_mode,
    void* config,
    vmi_init_error_t* error)
{
    return vmi_init_complete(vmi, domain, init_flags, init_data, config_mode, config, error);
}

status_t
fdrive_read_8_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint8_t* value)
{
    return vmi_read_8_va(vmi, vaddr, pid, value);
}

status_t
fdrive_read_16_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint16_t* value)
{
    return vmi_read_16_va(vmi, vaddr, pid, value);
}

status_t
fdrive_read_32_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint32_t* value)
{
    return vmi_read_32_va(vmi, vaddr, pid, value);
}

status_t
fdrive_read_64_va(
    vmi_instance_t vmi,
    addr_t vaddr,
    vmi_pid_t pid,
    uint64_t* value)
{
    return vmi_read_64_va(vmi, vaddr, pid, value);
}

status_t
fdrive_events_listen(vmi_instance_t vmi, uint32_t timeout)
{
    return vmi_events_listen(vmi, timeout);
}

status_t
fdrive_destroy(
    vmi_instance_t vmi)
{
    return vmi_destroy(vmi);
}
