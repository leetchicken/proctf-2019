#!/usr/bin/env python

import subprocess as sp

def generate_pow(salt):
    proc = sp.Popen(["./pow_md5.elf",salt],stdout=sp.PIPE)
    res = proc.communicate()
    return res[0].rstrip().decode()
