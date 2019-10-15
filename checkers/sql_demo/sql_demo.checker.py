#!/usr/bin/env python3
  
import sys
import traceback
import socket
import re
import os
from binascii import unhexlify
from random import randint

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110
PORT = 1433

def trace(message):
    print(message, file=sys.stderr)

def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)

def info():
    verdict(OK, "vulns: 1")

def exec_sql(s, sql):
    q = "%s\n" % re.sub('\n', ' ', sql)

    try:
        s.send(q.encode('utf-8'))
    except ConnectionRefusedError:
        verdict(DOWN, "Connection error", "Connection refused while sending `%s`" % sql)
    except socket.timeout:
        verdict(DOWN, "Timeout", "Connection timeout while sending `%s`" % sql)
    except OSError as e:
        if 'No route to host' in str(e):
            verdict(DOWN, "No route to host", "No route to host while sending `%s`" % sql)
        else:
            raise

def recv(s):
    data = ''
    while True:
        if ">" in data:
            return data.rstrip().rstrip('>').rstrip()

        try:
            data = data + s.recv(1024).decode('utf-8')
        except ConnectionRefusedError:
            verdict(DOWN, "Connection error", "Connection refused in recv")
        except socket.timeout:
            verdict(DOWN, "Timeout", "Connection timeout in recv")
        except OSError as e:
            if 'No route to host' in str(e):
                verdict(DOWN, "No route to host", "No route to host in recv")
            else:
                raise

def get_random_text():
    messages_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'messages.txt')
    with open(messages_path, 'r', encoding='utf-8') as f:
        messages = [line.rstrip() for line in f]
        return messages[randint(0, len(messages) - 1)].replace("'", "")

def get_random_word(text):
    words = [w for w in text.split() if re.match(r'^\w+$', w) and len(w) > 3]
    return words[randint(0, len(words) - 1)]

def check(args):
    if len(args) != 1:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for check()")
    host = args[0]
    trace("check(%s)" % host)

    s = socket.socket()
    s.settimeout(5)

    try:
        s.connect((host, PORT))
    except ConnectionRefusedError:
        verdict(DOWN, "Connection error", "Connection refused")
    except socket.timeout:
        verdict(DOWN, "Timeout", "Connection timeout")
    except OSError as e:
        if 'No route to host' in str(e):
            verdict(DOWN, "No route to host", "No route to host")
        else:
            raise

    trace("Waiting for `> `")
    recv(s)

    trace("Executing .thumbprint")
    sql = ".thumbprint"
    exec_sql(s, sql)
    recv(s)

    random_text = get_random_text()
    random_word = get_random_word(random_text)

    sql = "SELECT hex(fts3_tokenizer('t', fts3_tokenizer('simple')));CREATE VIRTUAL TABLE text USING fts3(tokenize=t);"
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)

    try:
        simple_tokenizer_addr = int.from_bytes(unhexlify(recv(s).strip('|')), byteorder='little')
    except:
        verdict(MUMBLE, "Unexpected query result", "Unexpected fts3_tokenizer query result: `%s`" % result)

    trace("Got simple tokenizer struct addr: %s" % hex(simple_tokenizer_addr))

    sql = "INSERT INTO text VALUES('%s');" % random_text
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    recv(s)

    sql = "SELECT * FROM text WHERE text MATCH '%s';" % random_word
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    result = recv(s).strip('|')
    if result != random_text:
         verdict(MUMBLE, "Unexpected query result", "Unexpected fts-query result: `%s`" % result)

    trace("Checking `/proc/self/maps` content")

    sql = ".read /proc/self/maps"
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    maps = recv(s)

    sqlite_base_addr = int(maps.splitlines()[7][:12], 16)
    if simple_tokenizer_addr - sqlite_base_addr != 1179648:
        verdict(MUMBLE, "Unexpected file content", "Unexpected `/proc/self/maps` file content, expected libsqlite3 base addr: %s, actual libsqlite3 base addr: %s" % (hex(simple_tokenizer_addr - 1179648), hex(sqlite_base_addr)))

    s.close()

    sys.exit(OK)

def put(args):
    if len(args) != 4:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for put()")
    host, flag_id, flag_data, vuln = args
    trace("put(%s, %s, %s, %s)" % (host, flag_id, flag_data, vuln))

    s = socket.socket()
    s.settimeout(5)

    try:
        s.connect((host, PORT))
    except ConnectionRefusedError:
        verdict(DOWN, "Connection error", "Connection refused")
    except socket.timeout:
        verdict(DOWN, "Timeout", "Connection timeout")
    except OSError as e:
        if 'No route to host' in str(e):
            verdict(DOWN, "No route to host", "No route to host")
        else:
            raise

    trace("Waiting for `> `")

    try:
        data = s.recv(1024).decode('utf-8')
    except socket.timeout:
        verdict(DOWN, "Timeout", "Timeout while receiving `> `")

    if data != '> ':
        verdict(MUMBLE, "Protocol error", "Expected `> `, received `%s`" % data)
 
    sql = "ATTACH DATABASE '%s' as db; CREATE TABLE db.flags (flag text NOT NULL); INSERT INTO db.flags VALUES('%s');\n" % (flag_id, flag_data)

    trace("Executing `%s`" % sql.rstrip())

    try:
        s.send(sql.encode('utf-8'))
    except socket.timeout:
        verdict(DOWN, "Timeout", "Timeout while sending query")

    try:
        data = s.recv(1024).decode('utf-8')
    except socket.timeout:
        verdict(DOWN, "Timeout", "Timeout while receiving insert response")

    if data != '> ':
        verdict(MUMBLE, "Protocol error", "Expected `> `, received `%s`" % data)

    s.close()

    sys.exit(OK)

def get(args):
    if len(args) != 4:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for get()")
    host, flag_id, flag_data, vuln = args
    trace("get(%s, %s, %s, %s)" % (host, flag_id, flag_data, vuln))

    s = socket.socket()
    s.settimeout(5)

    try:
        s.connect((host, PORT))
    except ConnectionRefusedError:
        verdict(DOWN, "Connection error", "Connection refused")
    except socket.timeout:
        verdict(DOWN, "Timeout", "Connection timeout")
    except OSError as e:
        if 'No route to host' in str(e):
            verdict(DOWN, "No route to host", "No route to host")
        else:
            raise

    trace("Waiting for `> `")

    try:
        data = s.recv(1024).decode('utf-8')
    except socket.timeout:
        verdict(DOWN, "Timeout", "Timeout while receiving `> `")

    if data != '> ':
        verdict(MUMBLE, "Protocol error", "Expected `> `, received `%s`" % data)
 
    sql = "ATTACH DATABASE '%s' as db; SELECT flag from db.flags;\n" % flag_id
    trace("Executing `%s`" % sql.rstrip())

    try:
        s.send(sql.encode('utf-8'))
    except socket.timeout:
        verdict(DOWN, "Timeout", "Timeout during put()")

    try:
        data = s.recv(1024).decode('utf-8')
    except socket.timeout:
        verdict(DOWN, "Timeout", "Timeout while receiving insert response")

    lines = data.splitlines()

    expected = '|%s|' % flag_data
    if len(lines) > 0 and lines[0] != expected:
        verdict(CORRUPT, "Can't get flag", "Expected `%s`, received `%s`" % (expected, lines[0]))

    s.close()

    sys.exit(OK)

def main(args):
    if len(args) == 0:
        verdict(CHECKER_ERROR, "No args")
    try:
        if args[0] == "info":
            info()
        elif args[0] == "check":
            check(args[1:])
        elif args[0] == "put":
            put(args[1:])
        elif args[0] == "get":
            get(args[1:])
        else:
            verdict(CHECKER_ERROR, "Checker error", "Wrong action: " + args[0])
    except Exception as e:
        verdict(CHECKER_ERROR, "Checker error", "Exception: %s" % traceback.format_exc())

if __name__ == "__main__":
    try:
        main(sys.argv[1:])
        verdict(CHECKER_ERROR, "Checker error (NV)", "No verdict")
    except Exception as e:
        verdict(CHECKER_ERROR, "Checker error (CE)", "Exception: %s" % e)
