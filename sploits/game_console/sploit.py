#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import os
import requests
import struct

SERVER_ADDR = "192.168.1.1:8000"

url = 'http://%s/auth' % (SERVER_ADDR)
r = requests.get(url)
if r.status_code != 200:
    exit(1)
authKey = struct.unpack('I', r.content)[0]

url = 'http://%s/notification?auth=%x' % (SERVER_ADDR, authKey)

username = "EvilHacker"
message = "Hi!"
notification = struct.pack('I', len(username))
notification += bytes(username, 'utf-8')
notification += struct.pack('I', len(message))
notification += bytes(message, 'utf-8')
notificationLen = len(notification)

requests.post(url=url, data=notification, headers={'Content-Type': 'application/octet-stream'})