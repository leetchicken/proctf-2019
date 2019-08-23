#!/usr/bin/env python3

import os
import sys
import traceback
import sqlite3
import pika
from random import randint
from identity_server_http_client import IdentityServerHttpClient

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

def trace(message):
    print(message, file=sys.stderr)

def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)

def init_db():
    db_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'users.db')
    if not os.path.exists(db_path):
        trace("SQLite db file '%s' does not exist, creating" % db_path)
        conn = sqlite3.connect(db_path)
        cur = conn.cursor()
        cur.execute('CREATE TABLE users (id VARCHAR PRIMARY KEY NOT NULL, username VARCHAR NOT NULL, password VARCHAR NOT NULL, created INT NOT NULL)')
        cur.execute('CREATE UNIQUE INDEX username_index ON users(username)')
        conn.commit()
        conn.close()
        trace("SQLite db file '%s' created" % db_path)

def is_user_exists(username):
    db_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'users.db')
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute('SELECT * FROM users WHERE username=?', (username,))
    row = cur.fetchone()
    conn.close()
    if row is None:
        return False
    return True

def add_user(flag_id, username, password):
    db_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'users.db')
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute("INSERT INTO users VALUES (?,?,?,strftime('%s','now'))", (flag_id, username, password))
    conn.commit()
    conn.close()

def get_random_username():
    usernames_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'usernames.txt')
    f = open(usernames_path, "r")
    usernames = f.read().splitlines()
    f.close()
    return usernames[randint(0, len(usernames) - 1)]

def info():
    verdict(OK, "vulns: 1:2")

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

    identity_server_client = IdentityServerHttpClient(host)

    password = flag_id

    if vuln == "1":
        username = flag_data.rstrip('=')

    if vuln == "2":
        username = get_random_username()
        while is_user_exists(username):
            trace("User '%s' already exists, appending random digit" % username)
            username = username + str(randint(0, 9))

        trace("Generated username '%s'" % username)
        add_user(flag_id, username, password)
        trace("Username '%s' saved to db" % username)

    trace("Registering user '%s'" % username)
    result, feedback_queue = identity_server_client.create_new_user(username, password)
    if result != OK:
        sys.exit(result)
    trace("User '%s' registered" % username)

    if vuln == "1":
        sys.exit(OK)

    if vuln == "2":
        # TODO publish message
        sys.exit(OK)

    verdict(CHECKER_ERROR, "Unknown vuln number for put()", "Unknown vuln number '%s' for put" % vuln)

def get(args):
    if len(args) != 4:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for get()")
    host, flag_id, flag_data, vuln = args
    trace("get(%s, %s, %s, %s)" % (host, flag_id, flag_data, vuln))

    if vuln == "1":
        username = flag_data.rstrip('=')
        password = flag_id

        credentials = pika.PlainCredentials(username, password)

        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(host, credentials=credentials))
            connection.close()
        except pika.exceptions.AMQPConnectionError as e:
            if str(e):
                trace_msg = "Connection error: %s" % e
            else:
                trace_msg = "Connection error"

            if '(403)' in str(e):
                verdict(CORRUPT, "Access refused", trace_msg)
            else:
                verdict(DOWN, "Connection error", trace_msg)

        sys.exit(OK)

    if vuln == "2":
        sys.exit(OK)

    verdict(CHECKER_ERROR, "Unknown vuln number for get()", "Unknown vuln number '%s' for get" % vuln)

def main(args):
    if len(args) == 0:
        verdict(CHECKER_ERROR, "No args")
    try:
        init_db()
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
