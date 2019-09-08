#!/usr/bin/env python3

import os
import sys
import traceback
import sqlite3
import pika
import json
import re
import uuid
import time
import ssl
from random import randint
from elasticsearch.exceptions import ConnectionError, AuthenticationException, AuthorizationException
from elasticsearch import Elasticsearch, RequestsHttpConnection
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
        cur.execute('CREATE TABLE users (id VARCHAR PRIMARY KEY NOT NULL, host VARCHAR NOT NULL, username VARCHAR NOT NULL, password VARCHAR NOT NULL, created INT NOT NULL)')
        cur.execute('CREATE UNIQUE INDEX username_index ON users(username, host)')
        conn.commit()
        conn.close()
        trace("SQLite db file '%s' created" % db_path)

def is_user_exists(host, username):
    db_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'users.db')
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute('SELECT * FROM users WHERE username=? AND host=?', (username, host))
    row = cur.fetchone()
    conn.close()
    if row is None:
        return False
    return True

def save_to_db(host, flag_id, username, password):
    db_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'users.db')
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute("INSERT INTO users VALUES (?,?,?,?,strftime('%s','now'))", (flag_id, host, username, password))
    conn.commit()
    conn.close()

def get_username_from_db(flag_id):
    db_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'users.db')
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute('SELECT username FROM users WHERE id=?', (flag_id,))
    row = cur.fetchone()
    conn.close()
    if row is None:
        return None
    return row[0]

def get_random_username():
    usernames_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'usernames.txt')
    f = open(usernames_path, 'r', encoding='utf-8')
    usernames = [line.rstrip() for line in f if re.match(r'^\w+$',line)]
    f.close()
    return usernames[randint(0, len(usernames) - 1)].lower()

def get_random_log(flag_data=None):
    access_log_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'access.log')
    f = open(access_log_path, "r")
    logs = f.read().splitlines()
    f.close()
    log_line = logs[randint(0, len(logs) - 1)]
    if flag_data:
        log_line = log_line[:-1] + " (Flag: " + flag_data + ")\""
    return log_line

def build_log_message(log_line):
    data = {}
    data[get_random_username()] = get_random_log()
    data['content'] = log_line
    data[get_random_username()] = get_random_log()
    return json.dumps(data)

def info():
    verdict(OK, "vulns: 1:4")

def check(args):
    if len(args) != 1:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for check()")
    host = args[0]
    trace("check(%s)" % host)

    username = get_random_username()
    while is_user_exists(host, username):
        trace("User '%s' already exists, appending random digit to username" % username)
        username = username + str(randint(0, 9))

    password = username

    trace("Generated username '%s'" % username)
    save_to_db(host, str(uuid.uuid4()), username, password)
    trace("Username '%s' saved to checker users.db" % username)

    identity_server_client = IdentityServerHttpClient(host)

    trace("Registering user '%s'" % username)
    result, feedback_queue = identity_server_client.create_new_user(username, password)
    if result != OK:
        sys.exit(result)
    trace("User '%s' registered" % username)

    msg = {}
    msg[get_random_username()] = get_random_log()
    msg['content'] = {}
    msg[get_random_username()] = get_random_log()
    msg_json = json.dumps(msg)

    credentials = pika.PlainCredentials(username, password)
    cxt = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
    ssl_options = pika.SSLOptions(context=cxt)

    try:
        connection = pika.BlockingConnection(pika.ConnectionParameters(host=host, port=5672, credentials=credentials, ssl_options=ssl_options))
        channel = connection.channel()

        logs_exchange = 'logs.' + username
        channel.basic_publish(logs_exchange, '', msg_json, pika.BasicProperties(type='Deer.Messages.LogData,Deer'))

        count = 0
        while count < 5:
            method_frame, header_frame, body = channel.basic_get(queue = feedback_queue, auto_ack=True)
            if method_frame:
                break
            count += 1
            time.sleep(1)
        connection.close()

        if not method_frame:
            verdict(CORRUPT, "Can't get error message")

        error_msg_str = body.decode('utf-8')
        if not 'Unexpected character encountered while parsing value' in error_msg_str:
            verdict(CORRUPT, "Unexpected error message", "Unexpected error message: %s" % error_msg_str)

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

def put(args):
    if len(args) != 4:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for put()")
    host, flag_id, flag_data, vuln = args
    trace("put(%s, %s, %s, %s)" % (host, flag_id, flag_data, vuln))

    identity_server_client = IdentityServerHttpClient(host)

    password = flag_id

    if vuln == "1":
        username = flag_data.rstrip('=').lower()

    if vuln == "2":
        username = get_random_username()
        while is_user_exists(host, username):
            trace("User '%s' already exists, appending random digit to username" % username)
            username = username + str(randint(0, 9))

        trace("Generated username '%s'" % username)
        save_to_db(host, flag_id, username, password)
        trace("Username '%s' saved to checker users.db" % username)

    trace("Registering user '%s'" % username)
    result, feedback_queue = identity_server_client.create_new_user(username, password)
    if result != OK:
        sys.exit(result)
    trace("User '%s' registered" % username)

    if vuln == "1":
        sys.exit(OK)

    if vuln == "2":
        msg_json = build_log_message(get_random_log(flag_data))

        credentials = pika.PlainCredentials(username, password)
        cxt = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
        ssl_options = pika.SSLOptions(context=cxt)

        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(host=host, port=5672, credentials=credentials, ssl_options=ssl_options))
            channel = connection.channel()

            logs_exchange = 'logs.' + username
            channel.basic_publish(logs_exchange, '', msg_json, pika.BasicProperties(type='Deer.Messages.LogData,Deer'))

            count = 0
            while count < 3:
                method_frame, header_frame, body = channel.basic_get(queue = feedback_queue, auto_ack=True)
                if method_frame:
                    break
                count += 1
                time.sleep(1)
            connection.close()

            if method_frame:
                verdict(MUMBLE, "Got an error message")
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

    verdict(CHECKER_ERROR, "Unknown vuln number for put()", "Unknown vuln number '%s' for put" % vuln)

def get(args):
    if len(args) != 4:
        verdict(CHECKER_ERROR, "Wrong args count", "Wrong args count for get()")
    host, flag_id, flag_data, vuln = args
    trace("get(%s, %s, %s, %s)" % (host, flag_id, flag_data, vuln))

    password = flag_id

    if vuln == "1":
        username = flag_data.rstrip('=').lower()

        credentials = pika.PlainCredentials(username, password)
        cxt = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
        ssl_options = pika.SSLOptions(context=cxt)

        try:
            connection = pika.BlockingConnection(pika.ConnectionParameters(host=host, port=5672, credentials=credentials, ssl_options=ssl_options))
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
        username = get_username_from_db(flag_id)
        es = Elasticsearch(['https://%s:9200' % host], http_auth=(username, password), verify_certs=False, ssl_show_warn=False, connection_class=RequestsHttpConnection)

        try:
            res = es.search(index=username, body={"query": { "match": { "content": { "query": "flag" } } }})
            flag_re = re.compile("Flag:\s+([^\)]+)")
            for hit in res['hits']['hits']:
                m = flag_re.search(hit['_source']['content'])
                if m.group(1) == flag_data:
                    sys.exit(OK)
        except ConnectionError as e:
            verdict(DOWN, "Connection error", e)
        except AuthenticationException as e:
            verdict(CORRUPT, "Authentication error", e)
        except AuthorizationException as e:
            verdict(CORRUPT, "Authorization error", e)

        verdict(CORRUPT, "Can't find flag")

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
