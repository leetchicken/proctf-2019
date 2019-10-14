#!/usr/bin/python3

import sys
import os
import hashlib
import subprocess

N = 32

os.chdir(os.path.dirname(os.path.realpath(__file__)))

try:
    os.mkdir("ssh_host_keys")
except FileExistsError:
    print("Remove ./ssh_host_keys dir first")
    sys.exit(1)


for i in range(1, N+1):
    key_file_name = "ssh_host_keys/cloud_node%d_key" % i
    subprocess.check_call(["ssh-keygen", "-h", "-t", "rsa", "-N", "",
                           "-f", key_file_name, "-C", "clouldnode%d" % i])

    pub_key_file_name = key_file_name + ".pub"

    pub_key = open(pub_key_file_name, "rb").read().strip()
    open("ssh_host_keys/known_hosts", "ab").write(b"10.60.%d.253 %s\n" % (i, pub_key))

