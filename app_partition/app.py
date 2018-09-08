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
Example Furnace app.
"""

import furnace_frontend as fe
import time


class App(object):
    def __init__(self, ctx):
        """
        Tenant's app constructor
        :param ctx: A dict, {'uuid': [Furnace-assigned app UUID], 'md': [optional femain.py supplied metadata]}.
        """
        fe.log("in example constructor")
        fe.set_name("example_app")
        self.i = 0

        e = {"event_type": fe.TIMER, "time_value": 1.0, "callback": self.timer_callback}
        fe.event_register(e)

        e = {"event_type": fe.BE, "callback": self.be_callback}
        fe.event_register(e)

        fe.log("leaving example constructor")

    def be_callback(self, ctx):
        """
        Called when messages arrive from the backend
        :param ctx: a ctx named tuple collections.namedtuple('MessageContent', 'type message')
             type: (string) This message was 'broadcast' to all apps or 'notify' to this app individually.
             message (string): Contents of message.
        :returns: a string reply to send back to the backend
        """
        fe.log(f"BE CALLBACK: {ctx}")
        fe.log(f"message: {ctx.message}")

    def timer_callback(self, ctx):
        """
        Called on timer timeouts
        :param ctx: Presently unused, passes as a static "timer triggered" string
        :returns: Nothing.
        """
        fe.log("in timer callback")
        self.i += 1
        if self.i > 4:
            fe.log("exiting...")
            fe.exit()
