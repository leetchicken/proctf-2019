#!/usr/bin/env python3

import subprocess as sp,sys,time,re
import string,random
from socket import *
from struct import *
import traceback
idx_read = 0
DEBUG = 0
def id_gen(size=8, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for x in range(size))
def read_until(s,c):
    global idx_read
    #print("Read idx %d"%idx_read)
    idx_read+=1
    cc = s.recv(1)
    mes=b""
    while cc != c:
        mes+=cc
#        sys.write(cc.decode())
        cc = s.recv(1)
        if len(cc)==0:
            break
    mes += cc
    if DEBUG:
        sys.stderr.write("<-"+str(mes))
    try:
        return mes.decode()
    except:
        return mes
def send_mes(s,mes):
    if (type(mes) == type(b"a")):
        mes += b"\n"
        if DEBUG:
            sys.stderr.write("->"+str(mes))
        s.send(mes)
    elif (type(mes) == type("a")):
        mes += "\n"
        if DEBUG:
            sys.stderr.write("->"+str(mes.encode()))
        s.send(mes.encode())
    else:
        print("Cannot encode")
    #s.flush()
    #time.sleep(0.5)

def AllocSpaceship(s,idx,name):
    send_mes(s,"1")
    read_until(s,b"\n")
    send_mes(s,"%d" % idx)
    read_until(s,b"\n")
    send_mes(s,"%s" %name)
    return read_until(s,b">")

def AllocSpaceman(s,idx,name,password):
    send_mes(s,"2")
    read_until(s,b"\n")
    send_mes(s,"%d" % idx)
    read_until(s,b"\n")
    send_mes(s,"%s" % name)
    read_until(s,b"\n")
    send_mes(s,"%s" % password)
    return read_until(proc,b">")

def EditSpaceship(s,idx,name,launch,access,pilot_idx,\
                            medic_idx, engineer_idx,Capitan_idx):
    res=""
    send_mes(s,"5")
    res+=read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    res+=read_until(s,b"\n")
    send_mes(s,"%s" %name)
    res+=read_until(s,b"\n")
    send_mes(s,"%s" %launch)
    res+=read_until(s,b"\n")
    res+=read_until(s,b"\n")
    send_mes(s,"%s" %access)
    res+=read_until(s,b"\n")
    res+=read_until(s,b"\n")
    send_mes(s,"%d" %pilot_idx)
    res+=read_until(s,b"\n")
    res+=read_until(s,b"\n")
    send_mes(s,"%d" %medic_idx)
    res+=read_until(s,b"\n")
    res+=read_until(s,b"\n")
    send_mes(s,"%d" %engineer_idx)
    res+=read_until(s,b"\n")
    res+=read_until(s,b"\n")
    send_mes(s,"%d" %Capitan_idx)
    res+=read_until(s,b">")
    return res
def EditSpaceman(s,idx,name,password):
    res=""
    send_mes(s,"8")
    res+=read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    res+=read_until(s,b"\n")
    send_mes(s,name)
    res+=read_until(s,b"\n")
    res+=read_until(s,b"\n")
    send_mes(s,password)
    res+=read_until(s,b">")
    return res
def SpacemanAddr(s,idx):
    send_mes(s,"255")
    cont = read_until(s,b">")
    all = re.findall(r'([0-9A-F]+)',cont)
    return int(all[idx],16)

def SpaceshipAddr(s,idx):
    send_mes(s,"254")
    cont = read_until(s,b">")
    all = re.findall(r'([0-9A-F]+)',cont)
    return int(all[idx],16)

def DeleteSpaceman(s,idx):
    send_mes(s,"4")
    read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    read_until(s,b">")

def DumpAddr(s,addr,qw):
    send_mes(s,"257")
    cont = read_until(s,b"\n")
    send_mes(s,"%d"%addr)
    cont = read_until(s,b"\n")
    send_mes(s,"%d"%qw)
    cont = read_until(s,b">")
    print(cont)
def ViewSpaceman(s,idx):
    send_mes(s,"7")
    read_until(s,b"\n")
    read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    a=read_until(s,b">")
    return a
def ViewSpaceship(s,idx):
    send_mes(s,"6")
    read_until(s,b"\n")
    read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    a=read_until(s,b">")
    return a
def ViewLaunch(s,idx,access):
    send_mes(s,"9")
    read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    read_until(s,b"\n")
    send_mes(s,access)
    a=read_until(s,b">")
    return a
def StoreSpaceship(s,idx):
    send_mes(s,"13")
    read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    return read_until(s,b">")
def LoadSpaceship(s,name):
    send_mes(s,"14")
    read_until(s,b"\n")
    send_mes(s,name)
    return read_until(s,b">")
def ViewLaunch(s,idx,acc):
    send_mes(s,"9")
    read_until(s,b"\n")
    send_mes(s,"%d"%idx)
    read_until(s,b"\n")
    send_mes(s,acc)
    return read_until(s,b">")
def SearchSpaceship(s,tmp):
    send_mes(s,"15")
    read_until(s,b"\n")
    read_until(s,b"\n")
    send_mes(s,tmp[:3])
    return read_until(s,b">")

if sys.argv[1] == "info":
    print("vulns: 1")
    exit(101)
#proc = sp.Popen("./serv12",shell=True,stdin=sp.PIPE,stdout=sp.PIPE)
proc = socket(AF_INET,SOCK_STREAM)
try:
    proc.connect((sys.argv[2],3777))
except OSError as e:
    traceback.print_tb(e.__traceback__)
    exit(104)
except Exception as e:
    traceback.print_tb(e.__traceback__)
    exit(104)
res = read_until(proc,b"\n")
res = read_until(proc,b">")

if sys.argv[1] == 'check':
    (u1,p1) = (id_gen(),id_gen())
    (u2,p2) = (id_gen(),id_gen())
    (u3,p3) = (id_gen(),id_gen())
    (u4,p4) = (id_gen(),id_gen())
    a1=AllocSpaceman(proc,0,u1,p1)
    a2=AllocSpaceman(proc,1,u2,p2)
    a3=AllocSpaceman(proc,2,u3,p3)
    a4=AllocSpaceman(proc,3,u4,p4)
    if DEBUG:
        sys.stderr.write(a1)
    if ("Successfuly allocated" not in a1 or \
            "Successfuly allocated" not in a2 or \
            "Successfuly allocated" not in a3 or \
            "Successfuly allocated" not in a4 ):
        exit(102)
    (nam1,la1,acc1) = (id_gen(),id_gen(),id_gen(7))
    a5 = AllocSpaceship(proc,0,nam1)
    if "Successfuly" not in a5:
        exit(102)
    a6=EditSpaceship(proc,0,"",la1,acc1,0,1,2,3)
    if "Successfuly set pilot" not in a6 or \
        "Successfuly set medic" not in a6 or \
        "Successfuly set engineer" not in a6 or \
        "Successfuly set capitan" not in a6 :
        exit(102)
    (u5,p5) = (id_gen(),id_gen())
    a7=EditSpaceman(proc,0,u5,p5)
    a8=ViewSpaceship(proc,0)
    if DEBUG:
        sys.stderr.write(a8)
    if "Pilot: %s" %u5 not in a8 or \
        "Medic: %s" %u2 not in a8 or \
        "Engineer: %s" %u3 not in a8 or \
        "Capitan: %s" %u4 not in a8:
        exit(102)
if sys.argv[1] == 'put':
    (u1,p1) = (id_gen(),id_gen())
    (u2,p2) = (id_gen(),id_gen())
    (u3,p3) = (id_gen(),id_gen())
    (u4,p4) = (id_gen(),id_gen())
    a1=AllocSpaceman(proc,0,u1,p1)
    a2=AllocSpaceman(proc,1,u2,p2)
    a3=AllocSpaceman(proc,2,u3,p3)
    a4=AllocSpaceman(proc,3,u4,p4)
    nam1 = id_gen()
    acc1 = id_gen(7)
    lac1 = sys.argv[4]
    a5 = AllocSpaceship(proc,0,nam1)
    a6=EditSpaceship(proc,0,"",lac1,acc1,0,1,2,3)
    a7 = StoreSpaceship(proc,0)
    a8 = SearchSpaceship(proc,nam1)
    if nam1 not in a8:
        exit(102)
    print(",".join([nam1,acc1]))

if sys.argv[1] == 'get':
    nam1,acc1 = sys.argv[3].split(",")
    flag = sys.argv[4]
    a8 = SearchSpaceship(proc,nam1)
    if nam1 not in a8:
        exit(102)
    a1 = LoadSpaceship(proc,nam1)
    a2 = ViewLaunch(proc,0,acc1)
    if flag not in a2:
        exit(102)

proc.close()
exit(101)
