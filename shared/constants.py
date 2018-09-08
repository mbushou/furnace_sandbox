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
Constants used by the Furnace app and facilities partitions.
"""

# passed by VMI partition
VMI_SUCCESS = 0
VMI_FAILURE = 1
# register types
CR3 = 1
CR4 = 2
# event types
REG = 3
INT = 4
MEM = 5
TIMER = 6
BE = 7
FE = 8
SYSCALL_PLUGIN = 9
# event status
ACTIVE = 20
INACTIVE = 21
# sync options
SYNC = 30
ASYNC = 31
PUBSUB = 32
# PID convenience
KERNEL = 0
# batching operators
F_ADD = 1
F_SUB = 2
F_OVERWRITE = 3

# input validation
ACCEPT_OPR = {F_ADD: "F_ADD", F_SUB: "F_SUB", F_OVERWRITE: "F_OVERWRITE"}
ACCEPT_SYNC = {SYNC: "F_SYNC", ASYNC: "F_ASYNC"}
ACCEPT_REG = {CR3: "F_CR3"}

FAC_IO_DIR = "/tmp/"

# poll timeouts
TIMEOUT_FAC = 250  # ms
TIMEOUT_VMI = 250  # ms

# max bytes size for protobufs
MAX_FIELD_SIZE = 11056943

# max disk usage
MAX_DISK_USAGE = 1024 * 1024 * 16  # 512MB
