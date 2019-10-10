#!/usr/bin/python3

import random
import os
import sys
import json

HOSTS_NUM = 256

HTTP_JURY_HOST = "10.10.10.120:8000"
HTTP2_JURY_HOST = "10.10.10.120:8001"


if os.path.exists("hosts.json"):
    print("Remove ./hosts.json first")
    sys.exit(1)


all_hosts = []
for t in range(1, 26):
    for last_octet in range(100, 200):
        all_hosts.append("10.60.%d.%d" % (t, last_octet))

all_ports = list(range(20000, 60000))

hosts = random.sample(all_hosts, HOSTS_NUM)
output = {"http": [], "http2": []}


iptables_add_file = open("iptables_add.txt", "w")
iptables_del_file = open("iptables_del.txt", "a")

for host in hosts:
    http_port, http2_port = random.sample(all_ports, 2)

    output["http"].append("%s:%d" % (host, http_port))
    output["http2"].append("%s:%d" % (host, http2_port))

    iptables_add_prefix = "iptables -t nat -A PREROUTING "
    iptables_del_prefix = "iptables -t nat -D PREROUTING "
    iptables_args_http = "-d %s/32 -p tcp -m tcp  --destination-port %d -j DNAT --to-destination %s" % (host, http_port, HTTP_JURY_HOST)
    iptables_args_http2 = "-d %s/32 -p tcp -m tcp  --destination-port %d -j DNAT --to-destination %s" % (host, http2_port, HTTP2_JURY_HOST)

    print(iptables_add_prefix + iptables_args_http, file=iptables_add_file)
    print(iptables_add_prefix + iptables_args_http2, file=iptables_add_file, flush=True)

    print(iptables_del_prefix + iptables_args_http, file=iptables_del_file)
    print(iptables_del_prefix + iptables_args_http2, file=iptables_del_file, flush=True)

json.dump(output, open("hosts.json", "w"))
