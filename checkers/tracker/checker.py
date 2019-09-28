#!/usr/bin/env python3
import logging
import os
import random
import string

import msgpack
import secrets
import socket
import sys

LOGS_DIR = "logs"
PORT = 9090
BUFFER_SIZE = 1024

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
    def __init__(self, host, port=PORT):
        sock = socket.socket(socket.AF_INET, SOCK_DCCP, IPROTO_DCCP)
        sock.setsockopt(SOL_DCCP, DCCP_SOCKOPT_SERVICE, True)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.connect((host, port))
        self.sock = sock

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

    def query_raw(self, obj):
        self.sock.send(msgpack.packb(obj))
        return msgpack.unpackb(self.sock.recv(BUFFER_SIZE), raw=False)


class ExitCode:
    OK = 101
    CORRUPT = 102
    MUMBLE = 103
    FAIL = 104
    INTERNAL_ERROR = 110


def _exit(code):
    logging.info("Exiting with code %d", code)
    sys.exit(code)


def create_login(length=16):
    letters = string.ascii_lowercase
    return "".join(random.choice(letters) for _ in range(length))


def check(ip):
    password = secrets.token_urlsafe(16)

    try:
        client = Client(ip)

        login = create_login()

        logging.info("Register user %r with password %r ...", login, password)
        client.query(Request.USER_REGISTER, login, password, expect=Response.OK)
        logging.info("Register succeeded")

        logging.info("Log in user %r with password %r ...", login, password)
        secret = client.query(Request.USER_LOGIN, login, password, expect=Response.OK)
        logging.info("Log in succeeded, secret: %r", secret)

        logging.info("Delete user with secret %r ...", secret)
        client.query(Request.USER_DELETE, secret, expect=Response.OK)
        logging.info("Delete succeeded")
    except AssertionError as e:
        logging.exception(e)
        _exit(ExitCode.MUMBLE)
    except ConnectionError as e:
        logging.exception(e)
        _exit(ExitCode.FAIL)

    _exit(ExitCode.OK)


def put(id, flag):
    pass


def get(id, flag):
    pass


def configure_logging(ip):
    os.makedirs(LOGS_DIR, exist_ok=True)
    log_filename = os.path.join(LOGS_DIR, "{}.log".format(ip))
    logging.basicConfig(level=logging.DEBUG,
                        filename=log_filename,
                        format="%(asctime)s %(levelname)-8s %(message)s")


def main():
    if len(sys.argv) < 3:
        print("Usage: {} (check|put|get) IP".format(sys.argv[0]))
        _exit(ExitCode.INTERNAL_ERROR)

    mode = sys.argv[1]
    args = sys.argv[2:]

    configure_logging(ip=sys.argv[2])
    logging.info("Started with arguments: %r", sys.argv)

    if mode == "check":
        check(*args)
    elif mode == "put":
        put(*args)
    elif mode == "get":
        put(*args)
    else:
        logging.error("Unknown MODE.")
        _exit(ExitCode.INTERNAL_ERROR)


if __name__ == "__main__":
    main()
