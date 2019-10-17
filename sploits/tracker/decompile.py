#!/usr/bin/env python
import marshal
import uncompyle6
import sys

with open(sys.argv[1], "rb") as f:
    raw = f.read()
    co = marshal.loads(raw)
    uncompyle6.code_deparse(co)
    print()
