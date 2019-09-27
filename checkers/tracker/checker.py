#!/usr/bin/env python3
import msgpack
import secrets
import socket
import sys

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


class Client:
    def __init__(self, host, port=PORT):
        sock = socket.socket(socket.AF_INET, SOCK_DCCP, IPROTO_DCCP)
        sock.setsockopt(SOL_DCCP, DCCP_SOCKOPT_SERVICE, True)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.connect((host, port))
        self.sock = sock

    def query(self, *args, expect=None):
        response = self.query_raw(args)
        if expect is not None:
            assert response[0] == expect
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


def check(ip):
    password = secrets.token_urlsafe(16)

    try:
        client = Client(ip)

        user_id = client.query(Request.USER_REGISTER, password, expect=Response.OK)
        secret = client.query(Request.USER_LOGIN, user_id, password, expect=Response.OK)
        client.query(Request.USER_LOGOUT, secret, expect=Response.OK)
    except AssertionError:
        sys.exit(ExitCode.MUMBLE)
    except ConnectionError:
        sys.exit(ExitCode.FAIL)

    sys.exit(ExitCode.OK)


def put(id, flag):
    pass


def get(id, flag):
    pass


def main():
    if len(sys.argv) < 3:
        print("Usage: {} MODE IP".format(sys.argv[0]))
        sys.exit(ExitCode.INTERNAL_ERROR)

    mode = sys.argv[1]
    args = sys.argv[2:]
    if mode == "check":
        check(*args)
    elif mode == "put":
        put(*args)
    elif mode == "get":
        put(*args)
    else:
        print("Error: unknown MODE.")
        sys.exit(ExitCode.INTERNAL_ERROR)


if __name__ == "__main__":
    main()
