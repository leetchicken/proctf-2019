#!/usr/bin/env python3

import sys
import random
import struct
import base64
import traceback

import nclib

from client import DepositClient

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 5555
TIMEOUT = 3


def create_session(host):
    client = DepositClient()

    conn = nclib.Netcat((host, PORT))
    conn.recvuntil(b'> ')
    conn.sendline(client.handshake())
    response = conn.recvuntil(b'> ')
    conn.sendline(client.handshake_response(response))
    response = conn.recvuntil(b'> ')

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
        conn.sendline(client.handle_admin_challenge(conn.recvuntil(b'> ')))
        results = client.parse_response(conn.recvuntil(b'> '))
    except Exception as e:
        verdict(MUMBLE, "Check failed", traceback.format_exc())

    verdict(OK)


def put(host, flag_id, flag, vuln):
    try:
        conn, client = create_session(host)
        location = client.pick_location(rce_flag=int(vuln) == 1)
        conn.sendline(client.store_secret(bytes(flag, "ascii"), location))
        response = client.parse_response(conn.recvuntil(b'> '))
        password = response.key
    except Exception as e:
        verdict(MUMBLE, "Failed to put flag", traceback.format_exc())

    flag_id = base64.b64encode(struct.pack("II", *location) + password).decode()
    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    try:
        conn, client = create_session(host)
        temp = base64.b64decode(flag_id)
        password = temp[8:]
        location = struct.unpack("II", temp[:8])
        conn.sendline(client.get_secret(location, password))
        response = client.parse_response(conn.recvuntil(b'> '))
        svc_flag = response.secret
    except Exception as e:
        verdict(MUMBLE, "Failed to get flag", traceback.format_exc())

    if bytes(flag, "ascii") != svc_flag[:32]:
        verdict(CORRUPT, "No flag to be seen", str(svc_flag))
    
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
    except Exception as E:
        verdict(CHECKER_ERROR, "Checker error", "Checker error: %s" % traceback.format_exc())
    verdict(CHECKER_ERROR, "Checker error", "No verdict")


if __name__ == "__main__":
    main(args=sys.argv[1:])
