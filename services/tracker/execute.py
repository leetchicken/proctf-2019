#!/usr/bin/env python3
import marshal
import sys

if len(sys.argv) < 2:
    print("Usage: {} FILE_NAME [ARGS ...]".format(sys.argv[0]), file=sys.stderr)
    sys.exit(1)

with open(sys.argv[1], "rb") as f:
    binary = marshal.load(f)
    del sys.argv[1]
    exec(binary)
