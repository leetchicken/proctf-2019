#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import os
import requests
import struct
import socket
import time
import random
import string

serverAddr = argv[1]
userName = argv[2]
password = argv[3]

url = 'http://%s:8000/auth?u=%s&p=%s' % (serverAddr, userName, password)
r = requests.get(url)
if r.status_code != 200:
    exit(1)
authKeyRaw = r.content
authKey = struct.unpack('I', authKeyRaw)[0]
print("authKey: %x" % authKey)

notifySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
notifySock.connect((serverAddr, 8001))
notifySock.send(authKeyRaw)

while True:
    try:
        data = notifySock.recv(16, socket.MSG_DONTWAIT)
        if len(data) == 0:
            print("Connection closed, reopen")
            notifySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            notifySock.connect((serverAddr, 8001))
            notifySock.send(authKeyRaw)
        else:
            notificationsNum = struct.unpack('IIII', data)[0]
            print("Notifications available: %u" % notificationsNum)
            notifySock.send(data)

            url = 'http://%s:8000/notification?auth=%x' % (serverAddr, authKey)
            r = requests.get(url)
            if r.status_code != 200:
                printf("Get notification failed")
            if len(r.content) > 0:
                print("Got notification")
    except:
        pass

    method = int(random.uniform(0, 6))
    if method == 0:
        url = 'http://%s:8000/auth?u=%s&p=%s' % (serverAddr, userName, password)
    if method == 1:
        url = 'http://%s:8000/list?auth=%x' % (serverAddr, authKey)
    if method == 2:
        gameIdx = int(random.uniform(1, 4))
        url = 'http://%s:8000/icon?auth=%x&id=%u' % (serverAddr, authKey, gameIdx)
    if method == 3:
        gameIdx = int(random.uniform(1, 4))
        url = 'http://%s:8000/code?auth=%x&id=%u' % (serverAddr, authKey, gameIdx)
    if method == 4:
        l_userName = ''.join(random.choice(string.ascii_uppercase + string.digits) for i in range(8))
        l_password = ''.join(random.choice(string.ascii_uppercase + string.digits) for i in range(8))
        url = 'http://%s:8000/register?u=%s&p=%s' % (serverAddr, l_userName, l_password)
        continue
    if method == 5:
        sender = ''.join(random.choice(string.ascii_uppercase + string.digits) for i in range(8))
        message = ''.join(random.choice(string.ascii_uppercase + string.digits) for i in range(64))

        notification = struct.pack('I', len(sender))
        notification += bytes(sender, 'utf-8')
        notification += struct.pack('I', len(message))
        notification += bytes(message, 'utf-8')

        url = 'http://%s:8000/notification?auth=%x' % (serverAddr, authKey)
        print(url)
        requests.post(url=url, data=notification, headers={'Content-Type': 'application/octet-stream'})

    if method != 5:
        print(url)
        r = requests.get(url)
        print(r.status_code)

    if r.status_code != 200:
        exit(1)

    if method == 0:
        authKeyRaw = r.content
        authKey = struct.unpack('I', authKeyRaw)[0]
        print("authKey: %x" % authKey)
        