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
import PIL.Image
from io import BytesIO

from Crypto.PublicKey import RSA
from Crypto.Signature import PKCS1_v1_5
from Crypto.Hash import SHA256

import paintings

from user_agents import USER_AGENTS

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 80
TIMEOUT = 3

SCRIPT_PATH = pathlib.Path(__file__).parent


signer = PKCS1_v1_5.new(RSA.importKey(open("private.pem").read()))


def get_team_num(host):
	m = re.search(r"\d+\.\d+\.(\d+)\.\d+", host)
	if not m:
		return None

	return m.group(1)

def parse_jpg_bytes(body):
    try:
        painting = PIL.Image.open(BytesIO(body))
    except Exception as e:
        return None
    return painting

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

    h = SHA256.new()
    h.update(host.encode())
    h.update(painting_bytes)
    sign = signer.sign(h)

    ans = session.put(url, data=painting_bytes, headers={"X_SIGN": base64.b64encode(sign)})
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not dict:
        return None
    return ans_obj

def call_post_replica_api(session, host, painting_id, replica_bytes):
    url = "http://%s:%d/replica?id=%s" % (host, PORT, painting_id)

    ans = session.post(url, data=replica_bytes)
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not dict:
        return None
    return ans_obj


def call_get_preview_api(session, host, painting_id):
    url = "http://%s:%d/preview?id=%s" % (host, PORT, painting_id)

    ans = session.get(url)
    if ans.status_code != 200:
        return None
    ans_bytes = ans.content
    return ans_bytes

def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)


def info():
    verdict(OK, "vulns: 1")


def check(host):
    if random.randint(0,1) == 0:
        print("Lucky! skipping check", file=sys.stderr)
        verdict(OK)

    start = time.time()
    s = create_session()
    ids = call_get_paintings_api(s, host)    
    elapsed = time.time() - start
    print(f"Done call_get_paintings_api in {elapsed}", file=sys.stderr)
    if ids is None:
        verdict(MUMBLE, "Can't get valid paintings list", f"Can't get valid paintings list")

    verdict(OK)

def put(host, flag_id, flag, vuln):
    s = create_session()

    file_name, painting_bytes = paintings.next_painting(get_team_num(host))

    start = time.time()
    ans = call_put_painting_api(s, host, flag, painting_bytes)
    elapsed = time.time() - start
    print(f"Done call_put_painting_api in {elapsed}", file=sys.stderr)

    if ans is None:
        verdict(MUMBLE, "Malformed response", "Malformed response: can't parse as expected")
    error = ans.get("error")
    if error is not None:
        verdict(MUMBLE, "Error from service", f"Error from service: {error}")

    painting_id = ans.get("id")
    if painting_id is None:
        verdict(MUMBLE, "Painting id not found", "Painting id not found")

    flag_id = base64.b64encode(json.dumps({"file": file_name, "id": painting_id}).encode()).decode()

    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    obj = json.loads(base64.b64decode(flag_id))
    file_name = obj["file"]
    painting_id = obj["id"]

    start = time.time()
    s = create_session()
    ids = call_get_paintings_api(s, host)
    elapsed = time.time() - start
    print(f"Done call_get_paintings_api in {elapsed}", file=sys.stderr)
    if ids is None or painting_id not in ids:
        verdict(MUMBLE, "Can't find painting id in list", "Can't find painting id in list")

    start = time.time()
    preview_bytes = call_get_preview_api(s, host, painting_id)
    elapsed = time.time() - start
    print(f"Done call_get_preview_api in {elapsed}", file=sys.stderr)
    if preview_bytes is None:
        verdict(MUMBLE, "Can't get preview", "Can't get preview")

    preview_img = parse_jpg_bytes(preview_bytes)
    if preview_img is None:
        verdict(MUMBLE, "Can't parse preview", "Can't parse preview")
    # TODO check preview image

    team_num = get_team_num(host)

    methods = [paintings.get_replica, paintings.get_copy, paintings.get_random_painting]
    random.shuffle(methods)

    for m in methods:
        name, painting_bytes = m(team_num, file_name)
        start = time.time()
        ans = call_post_replica_api(s, host, painting_id, painting_bytes)
        elapsed = time.time() - start
        print(f"{file_name}: sent file {name} in {elapsed}: ans {ans}", file=sys.stderr)
        if ans is not None and ans.get("reward") is not None:
            reward = ans.get("reward")
            if reward != flag:
                verdict(CORRUPT, "Got invalid reward", f"Got invalid reward: {reward}")
            else:
                verdict(OK)

    verdict(CORRUPT, "Can't get valid reward", "Can't get valid reward")

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
