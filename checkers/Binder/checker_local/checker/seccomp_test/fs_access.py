#!/usr/bin/env python3
from seccomp import *
import subprocess as sp
f = SyscallFilter(defaction=KILL)

f.add_rule(ALLOW, "write", Arg(0, EQ, sys.stdout.fileno()))
f.add_rule(ALLOW, "exit_group")
f.add_rule(ALLOW, "rt_sigaction")
f.add_rule(ALLOW, "brk")
f.load()


res = sp.Popen(["./fs_access.elf"],stdout=sp.PIPE)
print(res.communicate())
