#!/usr/bin/python3

import sys
import os
import secrets
import hashlib

N = 32

PRECREATED_TOKENS = [
"1_111e886db3424fd2a9dd453e6c8a6492",
"2_d8a4fe6d49b44459b0a73bfdeee7d60d",
"3_f3c68f2a5e194eab921864a2fff4767e",
"4_7352e83e72944ce48bb46125c67a2d2c",
"5_2414af92486241feb87e4ffdc8e2e296",
"6_3035226b28264fb1a84a4c427e58dfd2",
"7_fc73ab13c1ea4f4f8eff6a2d1851c061",
"8_ffc631d45be649e885e7b1e8681b06bf",
"9_4261b21cd024415ba4ee8aec24c8b735",
"10_22da646d58e745398e6a9bb24e17b45f",
"11_511ee8518f5040ec8e2ab770242964d5",
"12_284b1ff02d634128bd9af1d8784eb98f",
"13_096e4cbbc1a4484b8f569673e9af20e2",
"14_866c36db1bb0428b85f1835f2f8a7424",
"15_fbfaba9ef1274b7a8a5b76f8717aca7d",
"16_72dd7a910641433a9f56bd083038f76a",
"17_550cdc9de9a04dd5b4b6d2557d57c7e7",
"18_e770908770094a05996d38da3472cf93",
"19_3994529b396241938ea37403a87b1773",
"20_2e1c554239334a4faaf36fa186448820"
]

def gentoken(team, n=32):
 abc = "abcdef0123456789"
 return str(team) + "_" + "".join([secrets.choice(abc) for i in range(n)])

os.chdir(os.path.dirname(os.path.realpath(__file__)))

try:
    os.mkdir("tokens")
except FileExistsError:
    print("Remove ./tokens dir first")
    sys.exit(1)

try:
    os.mkdir("tokens_hashed")
except FileExistsError:
    print("Remove ./tokens_hashed dir first")
    sys.exit(1)

for i in range(1, N+1):
    try:
        token = PRECREATED_TOKENS[i-1]
    except IndexError:
        token = gentoken(i)
    token_hashed = hashlib.sha256(token.encode()).hexdigest()
    open("tokens/%d.txt" % i, "w").write(token + "\n")
    open("tokens_hashed/%d.txt" % i, "w").write(token_hashed + "\n")
