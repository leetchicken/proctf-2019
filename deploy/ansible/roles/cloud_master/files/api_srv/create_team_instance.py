#!/usr/bin/python3
# Developed by Alexander Bersenev from Hackerdom team, bay@hackerdom.ru

"""Creates vm instance for a team"""

import sys
import json
import time
import os
import traceback

from cloud_common import (get_cloud_ip, log_progress, call_unitl_zero_exit,
                          SSH_CLOUD_OPTS, get_vm_name_by_num)

TEAM = int(sys.argv[1])
VMNUM = int(sys.argv[2])


def log_stderr(*params):
    print("Team %d:" % TEAM, *params, file=sys.stderr)


def main():
    cloud_ip = get_cloud_ip(TEAM)
    if not cloud_ip:
        print("msg: ERR, no vm slots precreated")
        return 1

    vmname = get_vm_name_by_num(VMNUM)

    if not vmname:
        log_stderr("vm not found")
        return 1

    image_state = open("db/team%d/serv%d_image_deploy_state" % (TEAM, VMNUM)).read().strip()

    log_progress("5%")

    if image_state == "NOT_STARTED":
        file_from = "db/team%d/serv%d_root_passwd_hash.txt" % (TEAM, VMNUM)
        file_to = "%s:/home/cloud/serv%d_root_passwd_hash_team%d.txt" % (cloud_ip, VMNUM, TEAM)
        ret = call_unitl_zero_exit(["scp"] + SSH_CLOUD_OPTS + [file_from, file_to])
        if not ret:
            log_stderr("scp to CLOUD failed")
            return 1

        log_progress("25%")

        cmd = ["sudo", "/cloud/scripts/launch_vm.sh", str(TEAM), str(VMNUM), str(vmname)]
        ret = call_unitl_zero_exit(["ssh"] + SSH_CLOUD_OPTS + [cloud_ip] + cmd)
        if not ret:
            log_stderr("launch team vm")
            return 1

        image_state = "RUNNING"
        open("db/team%d/serv%d_image_deploy_state" % (TEAM, VMNUM), "w").write(image_state)
    
    log_progress("100%")
    return 0


if __name__ == "__main__":
    sys.stdout = os.fdopen(1, 'w', 1)
    print("started: %d" % time.time())
    exitcode = 1
    try:
        os.chdir(os.path.dirname(os.path.realpath(__file__)))
        exitcode = main()

        image_state = open("db/team%d/serv%d_image_deploy_state" % (TEAM, VMNUM)).read().strip()

        log_stderr("IMAGE_STATE:", image_state)

        if image_state != "RUNNING":
            print("msg: ERR, failed to start up the vm")
    except:
        traceback.print_exc()
    print("exit_code: %d" % exitcode)
    print("finished: %d" % time.time())
