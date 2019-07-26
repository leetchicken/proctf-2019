#!/usr/bin/env python

import subprocess as sp

def generate_pow(salt,pow_zero_len=3):
    proc = sp.Popen(["./pow_md5.elf",salt,str(pow_zero_len)],stdout=sp.PIPE)
    res = proc.communicate()
    return res[0].rstrip().decode()
