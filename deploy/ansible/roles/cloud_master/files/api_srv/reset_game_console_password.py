#!/usr/bin/python3
# Developed by Alexander Bersenev from Hackerdom team, bay@hackerdom.ru

"""Reset game console password"""

import sys
import time
import os
import traceback
import requests
import re

from cloud_common import (get_cloud_ip, log_progress, call_unitl_zero_exit,
                          get_vm_name_by_num, SSH_CLOUD_OPTS)

TEAM = int(sys.argv[1])
PASSWORD = sys.argv[2]


def log_stderr(*params):
    print("Team %d:" % TEAM, *params, file=sys.stderr)


def main():
    if not re.fullmatch(r"[a-z0-9A-Z]{,12}", PASSWORD):
        print("msg: ERR, bad char in password")
        return 1

    try:
        resp = requests.post("http://10.10.10.101/checksystem_change_password2?n=%d&p=%s" % (TEAM, PASSWORD))
        if resp.status_code == 200:
            print("msg: OK")
        else:
            print("msg: NOT_OK")
        log_stderr()
    except Exception:
        print("msg: ERR")
    return 0


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
