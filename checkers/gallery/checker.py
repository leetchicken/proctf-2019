#!/usr/bin/env python3

import sys
import requests
import hashlib
import random
import re
import functools
import time
import json
import pathlib
import base64
import traceback
#import PIL.Image
#from io import BytesIO

from user_agents import USER_AGENTS

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 10001
TIMEOUT = 3

SCRIPT_PATH = pathlib.Path(__file__).parent


def create_session():
    s = requests.Session()

    # add timeouts
    s.get = functools.partial(s.get, timeout=TIMEOUT)
    s.post = functools.partial(s.post, timeout=TIMEOUT)
    s.put = functools.partial(s.put, timeout=TIMEOUT)
    s.headers["User-Agent"] = random.choice(USER_AGENTS)
    return s


def call_get_paintings_api(session, host):
    url = "http://%s:%d/paintings" % (host, PORT)
    ans = session.get(url)
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not list or any(type(o) != str for o in ans_obj):
        return None
    return ans_obj


def call_put_painting_api(session, host, reward, painting_bytes):
    url = "http://%s:%d/painting?reward=%s" % (host, PORT, reward)

    ans = session.put(url, data=painting_bytes)
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not dict:
        return None    
    return ans_obj

def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)


def info():
    verdict(OK, "vulns: 1")


def check(host):
    s = create_session()

    ids = call_get_paintings_api(s, host)

    verdict(OK)

def put(host, flag_id, flag, vuln):
    s = create_session()

    ans = call_put_painting_api(s, host, flag, b'')
    if ans is None:
        verdict(MUMBLE, "Malformed response", "Malformed response: can't parse as expected")
    error = ans["error"]
    if error is not None:
    	verdict(MUMBLE, "Error from service", f"Error from service: {error}")

    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    s = create_session()

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
    except requests.RequestException as E:
        verdict(DOWN, "Connect error", "Connect error: %s" % E)
    except json.decoder.JSONDecodeError as E:
        verdict(MUMBLE, "Json decode error", "Json decode error: %s" % traceback.format_exc())
    except Exception as E:
        verdict(CHECKER_ERROR, "Checker error", "Checker error: %s" % traceback.format_exc())
    verdict(CHECKER_ERROR, "Checker error", "No verdict")


if __name__ == "__main__":
    main(args=sys.argv[1:])
