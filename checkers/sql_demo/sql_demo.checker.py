#!/usr/bin/env python3
  
import sys
import traceback
import socket

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

def check(args):
    if len(args) != 1:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for check()")
    host = args[0]
    trace("check(%s)" % host)
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
