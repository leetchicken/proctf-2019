#!/usr/bin/python3
# Developed by Alexander Bersenev from Hackerdom team, bay@hackerdom.ru

"""Shows all team instances

Recomended to be used by cloud administrators only
"""

import sys
import os
import traceback
import time
import re


def log_stderr(*params):
    print(*params, file=sys.stderr)


def log_team(team, *params):
    log_stderr("team %d:" % team, *params)


def get_vm_states():
    image_states = {}
    network_states = {}

    for filename in os.listdir("db"):
        m = re.fullmatch(r"team([0-9]+)", filename)
        if not m:
            continue
        team = int(m.group(1))
        try:
            image_state = open("db/%s/image_deploy_state" % (filename)).read().strip()
            image_states[team] = image_state

            network_state = open("db/%s/network_state" % (filename)).read().strip()
            network_states[team] = network_state
        except FileNotFoundError: 
            log_team(team, "failed to load states")
    return image_states, network_states


def get_cloud_ips():
    cloud_ips = {}

    for filename in os.listdir("db"):
        m = re.fullmatch(r"team([0-9]+)", filename)
        if not m:
            continue
        team = int(m.group(1))

        try:
            cloud_ip = open("db/%s/cloud_ip" % (filename)).read().strip()
            cloud_ips[team] = cloud_ip
        except FileNotFoundError:
            # it is ok, for undeployed VMs
            pass
    return cloud_ips



def main():
    image_states, network_states = get_vm_states()
    cloud_ips = get_cloud_ips()

    assert image_states.keys() == network_states.keys()
    teams = list(image_states.keys())


    print("%4s %16s %16s %s" % ("TEAM", "IMAGE_STATE", "NETWORK_STATE", "CLOUD_IP"))
    for team in sorted(teams):
        print("%4d %16s %16s %s" % (team, image_states[team], network_states[team], cloud_ips.get(team, "NO")))
            
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
