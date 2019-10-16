#!/usr/bin/env python3

import geocacher_pb2
import sys
import random
import struct
import base64
import traceback
import os

import nclib

from client import DepositClient, DepositClientError

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 5555
TIMEOUT = 3


def create_session(host):
    client = DepositClient(log=True)

    conn = nclib.Netcat((host, PORT), raise_timeout=True, retry=5)
    conn.settimeout(TIMEOUT)

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
        # 1. put temp secret
        secret = bytes(os.urandom(10).hex(), "ascii")
        conn, client = create_session(host)
        location = client.pick_location(rce_flag=False)
        conn.sendline(client.store_secret(secret, location))
        response = client.parse_response(conn.recvuntil(b'> '), geocacher_pb2.StoreSecretResponse)
        if response.status in (geocacher_pb2.FAIL, geocacher_pb2.TIMEOUT):
            verdict(DOWN, "Failed to put secret while checking", "Response status is %d" % response.status)
        if response.status != geocacher_pb2.OK:
            verdict(MUMBLE, "Failed to put secret while checking", "Response status is %d" % response.status)
        password = response.key
        conn.close()

        # 2. check if we see it in the list
        conn, client = create_session(host)
        conn.sendline(client.list_busy_cells())
        conn.sendline(client.handle_admin_challenge(conn.recvuntil(b'> ')))
        result = client.parse_response(conn.recvuntil(b'> '), geocacher_pb2.ListAllBusyCellsResponse)
        conn.close()
        for cell in result.cells:
            if (cell.coordinates.lat, cell.coordinates.lon) != location:
                continue
            if cell.secret.startswith(secret):
                break
        else:
            verdict(MUMBLE, "The item not found in the list", "Secret is not in the list")

        # 3. delete temp secret
        conn, client = create_session(host)
        conn.sendline(client.discard_secret(location, password))
        if response.status in (geocacher_pb2.FAIL, geocacher_pb2.TIMEOUT):
            verdict(DOWN, "Failed to delete secret while checking", "Response status is %d" % response.status)
        if response.status != geocacher_pb2.OK:
            verdict(MUMBLE, "Failed to delete secret while checking", "Response status is %d" % response.status)
        conn.close()
    except DepositClientError as e:
        verdict(MUMBLE, "Protocol error", traceback.format_exc())

    verdict(OK)


def put(host, flag_id, flag, vuln):
    try:
        conn, client = create_session(host)
        location = client.pick_location(rce_flag=int(vuln) == 1)
        conn.sendline(client.store_secret(bytes(flag, "ascii"), location))
        response = client.parse_response(conn.recvuntil(b'> '), geocacher_pb2.StoreSecretResponse)
        if response.status in (geocacher_pb2.FAIL, geocacher_pb2.TIMEOUT):
            verdict(DOWN, "Failed to put flag: db error", "Response status is %d" % response.status)
        if response.status != geocacher_pb2.OK:
            verdict(MUMBLE, "Failed to put flag", "Response status is %d" % response.status)
        password = response.key
    except DepositClientError as e:
        verdict(MUMBLE, "Failed to put flag: protocol error", traceback.format_exc())

    flag_id = base64.b64encode(struct.pack("II", *location) + password).decode()
    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    try:
        conn, client = create_session(host)
        temp = base64.b64decode(flag_id)
        password = temp[8:]
        location = struct.unpack("II", temp[:8])
        conn.sendline(client.get_secret(location, password))
        response = client.parse_response(conn.recvuntil(b'> '), geocacher_pb2.GetSecretResponse)
        if response.status == geocacher_pb2.NOT_FOUND:
            verdict(CORRUPT, "Failed to get flag: not found", "Response status is %d" % response.status)
        elif response.status != geocacher_pb2.OK:
            verdict(DOWN, "Failed to get flag: db error", "Response status is %d" % response.status)
        svc_flag = response.secret
    except DepositClientError as e:
        verdict(MUMBLE, "Failed to get flag: protocol error", traceback.format_exc())

    if not svc_flag.startswith(bytes(flag, "ascii")):
        verdict(CORRUPT, "Retrieved flag doesn't match", str(svc_flag))
    
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
    except nclib.NetcatError as e:
        verdict(DOWN, "Cannot establish connection", traceback.format_exc())
    except Exception as E:
        verdict(CHECKER_ERROR, "Checker error", traceback.format_exc())

    verdict(CHECKER_ERROR, "Checker error", "No verdict")


if __name__ == "__main__":
    main(args=sys.argv[1:])
