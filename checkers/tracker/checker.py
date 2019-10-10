#!/usr/bin/env python3
import hashlib
import hmac
import logging
import os
import random
import string

import msgpack
import base64
import socket
import sys

LOGS_DIR = "logs"
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


class ExitCode:
    OK = 101
    CORRUPT = 102
    MUMBLE = 103
    FAIL = 104
    INTERNAL_ERROR = 110


def create_random_string(length=16):
    letters = string.ascii_lowercase
    return "".join(random.choice(letters) for _ in range(length))


def create_password(login):
    pass_bytes = hmac.new(HMAC_PASS_KEY.encode(), msg=login.encode(), digestmod=hashlib.sha384).digest()
    return base64.b64encode(pass_bytes).decode()


def check(ip):
    login = create_random_string()
    password = create_password(login)

    login2 = create_random_string()
    password2 = create_password(login2)

    tracker_name = create_random_string(length=4)

    try:
        client = Client(ip)

        logging.info("Register first user %r with password %r ...", login, password)
        client.query(Request.USER_REGISTER, login, password, expect=Response.OK)
        logging.info("Register succeeded")

        logging.info("Log in first user %r with password %r ...", login, password)
        secret = client.query(Request.USER_LOGIN, login, password, expect=Response.OK)
        logging.info("Log in succeeded, secret: %r", secret)

        logging.info("Register second user %r with password %r ...", login2, password2)
        client.query(Request.USER_REGISTER, login2, password2, expect=Response.OK)
        logging.info("Register succeeded")

        logging.info("Log in second user %r with password %r ...", login2, password2)
        secret2 = client.query(Request.USER_LOGIN, login2, password2, expect=Response.OK)
        logging.info("Log in succeeded, secret: %r", secret)

        logging.info("Creating tracker %r ...", tracker_name)
        token = client.query(Request.TRACKER_ADD, secret, tracker_name, expect=Response.OK)
        points = [
            (1.2, 3.4, "f"),
            (5.6, 7.8, "o"),
            (9.0, 1.2, "o"),
        ]
        logging.info("Adding points to track ... ")
        track_id = client.query(Request.POINT_ADD_BATCH, token, points, expect=Response.OK)
        logging.info("Created track %r", track_id)

        logging.info("Second user asks for share track %r ...", track_id)
        client.query(Request.TRACK_REQUEST_SHARE, secret2, track_id, expect=Response.OK)

        logging.info("First user shares track %r ...", track_id)
        client.query(Request.TRACK_SHARE, secret, track_id, expect=Response.OK)

        logging.info("Second user tries to access shared track %r ...", track_id)
        client.query(Request.TRACK_GET, secret2, track_id, expect=Response.OK)

        logging.info("Delete first user with secret %r ...", secret)
        client.query(Request.USER_DELETE, secret, expect=Response.OK)
        logging.info("Delete succeeded")

        logging.info("Delete second user with secret %r ...", secret2)
        client.query(Request.USER_DELETE, secret2, expect=Response.OK)
        logging.info("Delete succeeded")

    except AssertionError as e:
        logging.exception(e)
        return ExitCode.MUMBLE
    except ConnectionError as e:
        logging.exception(e)
        return ExitCode.FAIL

    return ExitCode.OK


def put(ip, id, flag):
    login = id
    password = create_password(login)

    try:
        client = Client(ip)

        logging.info("Register user %r with password %r ...", login, password)
        client.query(Request.USER_REGISTER, login, password, expect=Response.OK)
        logging.info("Register succeeded")

        logging.info("Log in user %r with password %r ...", login, password)
        secret = client.query(Request.USER_LOGIN, login, password, expect=Response.OK)
        logging.info("Log in succeeded, secret: %r", secret)

        logging.info("Add tracker ...")
        token = client.query(Request.TRACKER_ADD, secret, "foo", expect=Response.OK)
        logging.info("Add tracker succeeded, token: %r", token)

        points = []
        for char in flag:
            lat = random.uniform(-10, 10)  # FIXME
            lon = random.uniform(-10, 10)
            points.append((lat, lon, char))
        logging.info("Add points %r ...", points)
        client.query(Request.POINT_ADD_BATCH, token, points, expect=Response.OK)
        logging.info("Add points succeeded")

    except AssertionError as e:
        logging.exception(e)
        return ExitCode.MUMBLE
    except ConnectionError as e:
        logging.exception(e)
        return ExitCode.FAIL

    return ExitCode.OK


def get(ip, id, flag):
    login = id
    password = create_password(login)

    try:
        client = Client(ip)

        logging.info("Log in user %r with password %r ...", login, password)
        secret = client.query(Request.USER_LOGIN, login, password, expect=Response.OK)
        logging.info("Log in succeeded, secret: %r", secret)

        logging.info("Get tracks ...")
        tracks = client.query(Request.TRACK_LIST, secret, expect=Response.OK)
        logging.info("Get tracks succeeded, they are: %r", tracks)

        flag_found = False

        for track in tracks:
            track_id = track[0]
            logging.info("Get track %r ...", track_id)
            points = client.query(Request.TRACK_GET, secret, track_id, expect=Response.OK)
            logging.info("Get track succeeded: %r", points)

            retrieved_flag = "".join([meta for ts, lat, lon, meta in points])
            logging.info("Retrieved flag: %r", retrieved_flag)

            if retrieved_flag == flag:
                logging.info("Flag FOUND!")
                flag_found = True
                break

        return ExitCode.OK if flag_found else ExitCode.CORRUPT

    except AssertionError as e:
        logging.exception(e)
        return ExitCode.MUMBLE
    except ConnectionError as e:
        logging.exception(e)
        return ExitCode.FAIL


def configure_logging(ip):
    os.makedirs(LOGS_DIR, exist_ok=True)
    log_filename = os.path.join(LOGS_DIR, "{}.log".format(ip))
    logging.basicConfig(level=logging.DEBUG,
                        filename=log_filename,
                        format="%(asctime)s %(levelname)-8s %(message)s")


def main():
    if len(sys.argv) < 3:
        print("Usage: {} (check|put|get) IP".format(sys.argv[0]))
        sys.exit(ExitCode.INTERNAL_ERROR)

    mode = sys.argv[1]
    args = sys.argv[2:]

    configure_logging(ip=sys.argv[2])
    logging.info("Started with arguments: %r", sys.argv)

    if mode == "check":
        exit_code = check(*args)
    elif mode == "put":
        exit_code = put(*args)
    elif mode == "get":
        exit_code = get(*args)
    else:
        exit_code = ExitCode.INTERNAL_ERROR
        logging.error("Unknown mode.")

    logging.info("Exiting with code %d", exit_code)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
