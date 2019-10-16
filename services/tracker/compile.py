#!/usr/bin/env python3
import marshal
import os
import sys

if len(sys.argv) != 3:
    print("Usage: {} IN_FILE OUT_FILE".format(sys.argv[0]))
    sys.exit(1)

in_file_name = sys.argv[1]
temp_file_name = "wrapper.py"
out_file_name = sys.argv[2]

with open(in_file_name) as f_in:
    with open(temp_file_name, "w") as f_temp:
        print("def wrapper():", file=f_temp)
        for line in f_in:
            print("    " + line, file=f_temp, end="")

from wrapper import wrapper
with open(out_file_name, "wb") as f_out:
    marshal.dump(wrapper.__code__, f_out)
print("{!r} file has been written.".format(out_file_name))

os.unlink(temp_file_name)
