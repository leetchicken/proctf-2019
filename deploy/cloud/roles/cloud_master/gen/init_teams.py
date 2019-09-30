#!/usr/bin/python3

import sys
import os
import shutil
import crypt

N = 32
S = 32

if __name__ != "__main__":
    print("I am not a module")
    sys.exit(0)

os.chdir(os.path.dirname(os.path.realpath(__file__)))

try:
    os.mkdir("db")
except FileExistsError:
    print("Remove ./db dir first")
    sys.exit(1)

for t in range(1, N+1):
    os.mkdir("db/team%d" % t)
    shutil.copyfile("tokens_hashed/%d.txt" % t, "db/team%d/token_hash.txt" % t)
    open("db/team%d/network_state" % t, "w").write("OPEN")

    for s in range(1, S+1):
        open("db/team%d/serv%d_image_deploy_state" % (t, s), "w").write("NOT_STARTED")

        root_passwd = open("root_passwds/passwds/team%d_serv%d_root_passwd.txt" % (t, s)).read().strip()
        root_passwd_hash = crypt.crypt(root_passwd, crypt.METHOD_SHA512)

        root_passwd_filename = "db/team%d/serv%d_root_passwd.txt" % (t,s)
        open(root_passwd_filename, "w").write(root_passwd + "\n")

        root_passwd_hash_filename = "db/team%d/serv%d_root_passwd_hash.txt" % (t,s)
        open(root_passwd_hash_filename, "w").write(root_passwd_hash + "\n")

    open("db/team%d/cloud_ip" % t, "w").write("10.60.%d.253\n" % t)
