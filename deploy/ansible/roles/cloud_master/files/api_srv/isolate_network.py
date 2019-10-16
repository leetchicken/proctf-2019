#!/usr/bin/python3
# Developed by Alexander Bersenev from Hackerdom team, bay@hackerdom.ru

"""Disconnects vm network from the game network"""

import sys
import time
import os
import traceback

from cloud_common import (get_cloud_ip, log_progress, call_unitl_zero_exit,
                          SSH_CLOUD_OPTS, ROUTER_HOST)

TEAM = int(sys.argv[1])


def log_stderr(*params):
    print("Team %d:" % TEAM, *params, file=sys.stderr)


def main():
    network_state = open("db/team%d/network_state" % TEAM).read().strip()


    if network_state == "OPEN":
        cmd = ["sudo", "/root/cloud/isolate_network.sh", str(TEAM)]
        ret = call_unitl_zero_exit(cmd)
        if not ret:
            log_stderr("isolate_network")
            return 1

        network_state = "ISOLATED"
        open("db/team%d/network_state" % TEAM, "w").write(network_state)

    if network_state == "ISOLATED":
        print("msg: OK")
        return 0
    return 1

if __name__ == "__main__":
    sys.stdout = os.fdopen(1, 'w', 1)
    print("started: %d" % time.time())
    exitcode = 1
    try:
        os.chdir(os.path.dirname(os.path.realpath(__file__)))
        exitcode = main()
    except:
        traceback.print_exc()
    print("exit_code: %d" % exitcode)
    print("finished: %d" % time.time())
