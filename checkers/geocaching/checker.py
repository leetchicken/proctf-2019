#!/usr/bin/env python3

import sys
import random
import struct
import base64
import traceback

from pwn import *
from client import DepositClient

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 5555
TIMEOUT = 3


def create_session(host):
    context.log_console=open('/dev/null', 'w')
    context.timeout = TIMEOUT
    client = DepositClient()

    conn = remote(host, PORT)
    conn.recvuntil('> ')
    conn.sendline(client.handshake())
    response = conn.recvuntil('> ', drop=True)
    conn.sendline(client.handshake_response(response))
    response = conn.recvuntil('> ', drop=True)

    return conn, client


def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)


def info():
    verdict(OK, "vulns: 4:1")


def check(host):
    try:
        conn, client = create_session(host)
        conn.sendline(client.list_busy_cells())
        conn.sendline(client.handle_admin_challenge(conn.recvuntil('> ', drop=True)))
        results = client.parse_response(conn.recvuntil('> ', drop=True))
    except Exception as e:
        verdict(MUMBLE, "Check failed", "Something doesn't work ¯\\_(ツ)_/¯")

    verdict(OK)


def put(host, flag_id, flag, vuln):
    try:
        conn, client = create_session(host)
        location = client.pick_location(rce_flag=int(vuln) == 1)
        conn.sendline(client.store_secret(bytes(flag, "ascii"), location))
        response = client.parse_response(conn.recvuntil('> ', drop=True))
        password = response.key
    except Exception as e:
        verdict(MUMBLE, "Failed to put flag", "Something doesn't work ¯\\_(ツ)_/¯")

    flag_id = base64.b64encode(struct.pack("II", *location) + password).decode()
    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    try:
        conn, client = create_session(host)
        temp = base64.b64decode(flag_id)
        password = temp[8:]
        location = struct.unpack("II", temp[:8])
        conn.sendline(client.get_secret(location, password))
        response = client.parse_response(conn.recvuntil('> ', drop=True))
        svc_flag = response.secret
    except Exception as e:
        verdict(MUMBLE, "Failed to get flag", "Mumble-huyambl ¯\\_(ツ)_/¯")

    if bytes(flag, "ascii") != svc_flag:
        verdict(CORRUPT, "No flag to be seen")
    
    verdict(OK)


def main(args):
    CMD_MAPPING = {
        "info": (info, 0),
        "check": (check, 1),
        "put": (put, 4),
        "get": (get, 4),
    }

    if not args:
        verdict(CHECKER_ERROR, "No args", "No args")

    cmd, args = args[0], args[1:]
    if cmd not in CMD_MAPPING:
        verdict(CHECKER_ERROR, "Checker error", "Wrong command %s" % cmd)

    handler, args_count = CMD_MAPPING[cmd]
    if len(args) != args_count:
        verdict(CHECKER_ERROR, "Checker error", "Wrong args count for %s" % cmd)

    try:
        handler(*args)
    except pwnlib.exception.PwnlibException as E:
        verdict(DOWN, "Connect error", "Connect error: %s" % E)
    except Exception as E:
        verdict(CHECKER_ERROR, "Checker error", "Checker error: %s" % traceback.format_exc())
    verdict(CHECKER_ERROR, "Checker error", "No verdict")


if __name__ == "__main__":
    main(args=sys.argv[1:])

# '172.16.86.130'