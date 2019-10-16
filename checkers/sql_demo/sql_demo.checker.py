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
    thumbprint = recv(s)

    trace("Checking sqlite3_exec")
    exec_sql(s, "CREATE TABLE t(a INT);BEGIN;")
    r = recv(s)
    if r != '':
        verdict(MUMBLE, "Unexpected query result", "Unexpected create table result: `%s`" % r)
    exec_sql(s, "INSERT INTO t VALUES(%s); ROLLBACK;SELECT COUNT(*) FROM t;" % randint(0, 6555663))

    try:
        count = int(recv(s).strip('|'))
    except:
        verdict(MUMBLE, "Unexpected query result", "Exception while parse count")

    if count != 0:
        verdict(MUMBLE, "Unexpected query result", "Unexpected count query result: `%s`" % count)

    sql = """
    CREATE TABLE x(id integer primary key, a TEXT NULL);
    INSERT INTO x (a) VALUES ('first');
    CREATE TABLE tempx(id integer primary key, a TEXT NULL);
    INSERT INTO tempx (a) VALUES ('t-first');
    CREATE VIEW tv1 AS SELECT x.id, tx.id FROM x JOIN tempx tx ON tx.id=x.id;
    CREATE VIEW tv1b AS SELECT x.id, tx.id FROM x JOIN tempx tx on tx.id=x.id;
    CREATE VIEW tv2 AS SELECT * FROM tv1 UNION SELECT * FROM tv1b;
    SELECT * FROM tv2;
    """
    exec_sql(s, sql)
    r = recv(s)
    if r != '|1|1|':
        verdict(MUMBLE, "Unexpected query result", "Unexpected query 1 result: `%s`" % r)

    sql = """
    create temp table t1(x);
    insert into t1 values('amx');
    insert into t1 values('anx');
    insert into t1 values('amy');
    insert into t1 values('bmy');
    select * from t1 where x like 'a__'
      intersect select * from t1 where x like '_m_'
      intersect select * from t1 where x like '__x';
    """
    exec_sql(s, sql)
    r = recv(s)
    if r != '|amx|':
        verdict(MUMBLE, "Unexpected query result", "Unexpected query 2 result: `%s`" % r)

    sql = """
    CREATE TABLE t01(x, y);
    CREATE TABLE t02(x, y);
    """
    exec_sql(s, sql)
    r = recv(s)
    if r != '':
        verdict(MUMBLE, "Unexpected query result", "Unexpected query 3 result: `%s`" % r)

    sql = """
    CREATE VIEW v0 as SELECT x, y FROM t01 UNION SELECT x FROM t02;
    EXPLAIN QUERY PLAN SELECT * FROM v0 WHERE x='0' OR y;
    """
    exec_sql(s, sql)
    r = recv(s)
    if r != 'E! SELECTs to the left and right of UNION do not have the same number of result columns':
        verdict(MUMBLE, "Unexpected query result", "Unexpected query 4 result: `%s`" % r)

    sql = "SELECT X'01020k304', 100;"
    exec_sql(s, sql)
    r = recv(s)
    if r != "E! unrecognized token: \"X'01020k304'\"":
        verdict(MUMBLE, "Unexpected query result", "Unexpected query 5 result: `%s`" % r)

    sql = """
    CREATE TABLE j2(id INTEGER PRIMARY KEY, json, src);
    INSERT INTO j2(id,json,src)
    VALUES(1,'{
      "firstName": "John",
      "lastName": "Smith",
      "isAlive": true,
      "age": 25,
      "address": {
        "streetAddress": "21 2nd Street",
        "city": "New York",
        "state": "NY",
        "postalCode": "10021-3100"
      },
      "phoneNumbers": [
        {
          "type": "home",
          "number": "212 555-1234"
        },
        {
          "type": "office",
          "number": "646 555-4567"
        }
      ],
      "children": [],
      "spouse": null
    }','https://en.wikipedia.org/wiki/JSON');

    SELECT count(*) FROM j2;
    """
    exec_sql(s, sql)
    r = recv(s)
    if r != '|1|':
        verdict(MUMBLE, "Unexpected query result", "Unexpected query 6 result: `%s`" % r)

    while True:
        random_text = get_random_text()
        if len(random_text) > 20:
            break

    random_word = get_random_word(random_text)

    sql = "CREATE VIRTUAL TABLE messages USING fts5(body);"
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    r = recv(s)
    if r != '':
        verdict(MUMBLE, "Unexpected query result", "Unexpected create fts table result: `%s`" % r)

    sql = "INSERT INTO messages VALUES('%s');" % random_text
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    r = recv(s)
    if r != '':
        verdict(MUMBLE, "Unexpected query result", "Unexpected insert result: `%s`" % r)

    sql = "SELECT * FROM messages WHERE body MATCH '%s';" % random_word
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    result = recv(s).strip('|')
    if result != random_text:
         verdict(MUMBLE, "Unexpected query result", "Unexpected fts-query result: `%s`" % result)

    sql = ".read /etc/passwd"
    trace("Executing `%s`" % sql)
    exec_sql(s, sql)
    recv(s)

    exec_sql(s, ".quit")
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
