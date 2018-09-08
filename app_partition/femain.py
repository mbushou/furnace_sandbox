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
Command line interface to start a tenant app.
"""

import sys
import os

sys.path.insert(0, os.path.abspath("/opt/furnace/frontend"))
sys.path.insert(0, os.path.abspath("/opt/furnace/shared"))
sys.path.insert(0, os.path.abspath("/opt/furnace/app"))

# furnace
import furnace_frontend as fe

# pre-loaded for apps
import collections
import math
import time
import random
import json
import pickle
import cProfile
import gc
import timeit
import argparse


def main():
    """
    Parses command line input, sets up env, then runs the tenant's app.
    """

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-c",
        dest="component",
        default="app",
        metavar="component",
        help="Component FILE without the .py extension, i.e., for app.py this would be app (default: app)",
    )
    parser.add_argument(
        "-m",
        dest="module",
        default="App",
        metavar="module",
        help="Python MODULE inside that file (default: App)",
    )
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
        "--md",
        dest="md",
        default="None",
        metavar="metadata",
        help="Metadata to pass as ctx into app",
    )
    parser.add_argument(
        "--fac",
        dest="fac",
        default="/opt/furnace/furnace_sandbox/tenant_data",
        metavar="path",
        help="Path to facilities partition sockets",
    )
    parser.add_argument(
        "--vmi",
        dest="vmi",
        default="/opt/furnace/furnace_sandbox/tenant_data",
        metavar="path",
        help="Path to VMI partition sockets",
    )
    args = parser.parse_args()
    print(args)

    target_component = args.component
    target_module = args.module
    debug = args.debug
    u2 = args.u2
    md = args.md

    if not os.path.isdir(args.fac):
        print(f"ERROR: {args.fac} is not a directory!")
        sys.exit()
    if not os.path.isdir(args.vmi):
        print(f"ERROR: {args.vmi} is not a directory!")
        sys.exit()

    # can pass info directly to app here
    ctx = {"uuid": u2, "metadata": md}

    # start up the frontend api and let it initialize
    fei = fe.FE(u2, args.fac, args.vmi, debug=debug)
    fei.module_register()

    # signal to syscall inspector to strengthen its policy
    try:
        fh = open("ESTABLISHED", "r")
    except FileNotFoundError as e:
        print(f"this is ok: {e}")
    print("established")
    # end signal

    # start tenant code
    # ALL SEC MECHANISMS MUST BE ACTIVE AT THIS POINT
    comp = __import__(target_component, fromlist=[target_module])
    appClass = getattr(comp, target_module)
    app = appClass(ctx)

    # double check the tenant app is sane
    fei.post_app_init(app=app)

    # loop forever
    fei.loop()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("caught keyboard interrupt")
