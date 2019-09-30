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

from user_agents import USER_AGENTS
import code_gen
import fraud_detector

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 10000
TIMEOUT = 3

SCRIPT_PATH = pathlib.Path(__file__).parent
USERS = json.load(open(SCRIPT_PATH / "users.json"))


def gen_rule_name():
    ABC = "".join(chr(i) for i in range(33, 127) if chr(i) != "/")
    name = "".join(random.choice(ABC) for i in range(random.randrange(6, 10)))

    if random.random() < 0.1:
        return name + ".py"
    if random.random() < 0.1:
        return "select * from " + name
    if random.random() < 0.1:
        return "' union select * from " + name
    if random.random() < 0.1:
        return "echo $" + name
    if random.random() < 0.1:
        return "`echo" + name + "`"
    if random.random() < 0.1:
        return name + ".txt"
    return name


def create_session():
    s = requests.Session()

    # add timeouts
    s.get = functools.partial(s.get, timeout=TIMEOUT)
    s.post = functools.partial(s.post, timeout=TIMEOUT)
    s.headers["User-Agent"] = random.choice(USER_AGENTS)
    return s


def call_get_rules_api(session, host):
    url = "http://%s:%d/rules" % (host, PORT)
    ans = session.get(url)
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not list or any(type(o) != str for o in ans_obj):
        return None
    return ans_obj


def call_add_rule_api(session, host, name, code):
    url = "http://%s:%d/addrule" % (host, PORT)
    ans = session.post(url, data=json.dumps({"name": name, "code": code}))
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not str:
        return None
    return ans_obj


def call_check_user_api(session, host, rules, user):
    url = "http://%s:%d/checkuser" % (host, PORT)
    ans = session.post(url, data=json.dumps({"rules": rules, "user": user}))
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not list or any(type(o) != int for o in ans_obj):
        return None
    return ans_obj


def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)


def info():
    verdict(OK, "vulns: 1:1:2")


def check(host):
    s = create_session()

    rule1_name = gen_rule_name()
    rule2_name = gen_rule_name()
    rule1 = code_gen.gen_empty_check()
    rule2 = code_gen.gen_rand_check()

    base_url = "http://%s:%d" % (host, PORT)

    for rule_name, rule in [rule1_name, rule1], [rule2_name, rule2]:
        ans = call_add_rule_api(s, host, name=rule_name, code=rule)
        if ans is None or not ans.startswith("ok:"):
            verdict(MUMBLE, "Failed to add rule",
                    "Failed to add rule: %s %s" % (rule_name, ans))

    ans = call_get_rules_api(s, host)
    if ans is None or rule1_name not in ans or rule2_name not in ans:
        verdict(MUMBLE, "Bad rule list", "Bad rule list: no new rules")

    user_idxs = random.sample(range(len(USERS)), 3)

    rules_seq = [random.choice([rule1_name, rule2_name]) for i in range(random.randint(32, 64))]

    ans = call_check_user_api(s, host, rules=rules_seq, user=user_idxs[0])
    if ans is None or len(set(ans)) in [1, 2]:
        verdict(MUMBLE, "Check failed", "Bad random test")

    for user_idx in user_idxs:
        ans = call_check_user_api(s, host, user=user_idx, rules=[rule1_name])

        expected = fraud_detector.run_rules([rule1], USERS[user_idx])
        if ans is None or expected != ans:
            verdict(MUMBLE, "Check failed", "Bad interpreter test")
    verdict(OK)


def put(host, flag_id, flag, vuln):
    s = create_session()

    rule_name = gen_rule_name()
    if int(vuln) == 1:
        rule = code_gen.gen_vuln1_check(flag)
    elif int(vuln) == 2:
        rule = code_gen.gen_vuln2_check(flag)
    else:
        rule = code_gen.gen_vuln3_check(flag)

    ans = call_add_rule_api(s, host, name=rule_name, code=rule)
    if ans is None or not ans.startswith("ok:"):
        verdict(MUMBLE, "Failed to add rule",
                "Failed to add rule: %s %s" % (rule_name, ans))

    user_idxs = random.sample(range(len(USERS)), 8)

    get_help_data = []
    for user_idx in user_idxs:
        expected = fraud_detector.run_rules([rule], USERS[user_idx])
        get_help_data.append([user_idx, expected])
    flag_id = base64.b64encode(json.dumps([rule_name, get_help_data]).encode()).decode()
    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    s = create_session()

    try:
        rule_name, get_help_data = json.loads(base64.b64decode(flag_id))
    except Exception:
        verdict(MUMBLE, "Bad flag id", "Bad flag_id: %s" % traceback.format_exc())

    ans = call_get_rules_api(s, host)
    if ans is None or rule_name not in ans:
        verdict(MUMBLE, "Bad rule list", "Bad rule list: no new rules")

    for user_idx, expected in random.sample(get_help_data, 4):
        ans = call_check_user_api(s, host, rules=[rule_name], user=user_idx)
        if ans is None:
            verdict(MUMBLE, "Check failed")
        if ans != expected:
            verdict(MUMBLE, "No such flag")
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
