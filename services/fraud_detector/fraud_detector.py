#!/usr/bin/python3
"The rule engine for fraud detector service"

import ast
import sys
import resource
import hashlib
import random
import signal
import concurrent.futures

SAFE_FUNCTIONS = {
    "locals": locals,
    "globals": globals,
    "type": type,
    "object": object,
    "bool": bool,
    "str": str,
    "int": int,
    "list": list,
    "set": set,
    "dict": dict,
    "bytes": bytes,
    "ord": ord,
    "chr": chr,
    "randint": random.randint,
    "sha256": lambda data: hashlib.sha256(data.encode()).digest()
}

SAFE_OBJS = {
    True: True,
    False: False,
    None: None,
}

BASE_GLOBS = {
    "__builtins__": {**SAFE_FUNCTIONS, **SAFE_OBJS}
}


def set_limits():
    resource.setrlimit(resource.RLIMIT_DATA, (10_000_000, 10_000_000))
    resource.setrlimit(resource.RLIMIT_CPU, (1, 1))
    signal.setitimer(signal.ITIMER_VIRTUAL, 2.0)  # finer grain control


class SecurityError(Exception):
    pass


class ASTSecurityChecker(ast.NodeVisitor):
    VISIT_BLACKLIST = ["Import", "ImportFrom", "Attribute"]

    def __getattr__(self, attr):
        if attr.startswith("visit_"):
            node_type = attr[len("visit_"):]
            if node_type in self.VISIT_BLACKLIST:
                raise SecurityError(f"{node_type} is not permitted")
            return getattr(super(), attr)


def run_rule(rule, user):
    globs = BASE_GLOBS.copy()
    globs.update({
        "user": user,
        "fraud_prob": 0
    })

    ast_rule = ast.parse(rule)
    ASTSecurityChecker().visit(ast_rule)

    code = compile(ast_rule, filename="rule.py", mode="exec")
    exec(code, globs)
    fraud_prob = min(100, max(0, globs["fraud_prob"]))

    return fraud_prob


def run_rules(rules, user):
    return [run_rule(rule, user) for rule in rules]


def run_rules_safe(rules, user):
    WAIT_TIMEOUT = 2.0
    executor = concurrent.futures.ProcessPoolExecutor(max_workers=1, initializer=set_limits)
    future = executor.submit(run_rules, rules, user)
    return future.result(timeout=WAIT_TIMEOUT)


if __name__ == "__main__":
    BASE_GLOBS["print"] = print

    rule = r"""
print("hello, world")
if randint(0, 2) == 0:
    fraud_prob = 42
"""
    print(run_rules_safe([rule], {"name": "test_name", "surname": "test_surname"}))
