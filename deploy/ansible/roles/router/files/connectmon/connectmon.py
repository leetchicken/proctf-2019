#!/usr/bin/env python3

import re
import sys
import collections
import socket
import subprocess
import traceback
import time

TEAMS_NUM = 32

CLIENT_TIMEOUT = 5
CHILD_WAIT_TIMEOUT = 5

JURY_NET_PREFIXES = ["10.10.10.", "127.0.0."]

CONNTRACK_ARGS = ["conntrack", "-L"]

LINE_RE = re.compile(r"""
    ^
        (tcp|udp|dccp)\s+
        (?#protocol_code)(?:6|17|33)\s+
        (?#data_len)(\d+)\s+
        (?#state)(?:(\w+)\s+)?
        src=(\d+\.\d+\.\d+\.\d+)\s+
        dst=(\d+\.\d+\.\d+\.\d+)\s+
        sport=\d+\s+
        dport=\d+\s+
        (?:\[UNREPLIED\]\s+)?
        src=(?:\d+\.\d+\.\d+\.\d+)\s+
        dst=(?:\d+\.\d+\.\d+\.\d+)\s+
        sport=\d+\s+
        dport=\d+\s+
        (?:\[ASSURED\]\s+)?
        mark=\d+\s+
        use=\d+\s*
    $
""", re.VERBOSE)

TEAM_NAMES = collections.defaultdict(str, {
    0: "jury",
    1: "team1",
    2: "team2",
    3: "team3",
    4: "team4",
    5: "team5",
    6: "team6",
    7: "team7",
    8: "team8",
    9: "team9",
    10: "team10",
    11: "team11",
    12: "team12",
    13: "team13",
    14: "team14",
    15: "team15",
    16: "team16",
    17: "team17",
    18: "team18",
    19: "team19",
    20: "team20",
    21: "team21",
    22: "team22",
    23: "team23",
    24: "team24",
    25: "team25",
    26: "team26",
    27: "team27",
    28: "team28",
    29: "team29",
    30: "team30",
    31: "team31",
})


def ip_to_team(ip):
    m = re.match(r"10\.60\.(\d+)\.\d+", ip)
    if not m:
        # jury is a team 0
        if re.match(r"10\.10\.10\.\d+", ip):
            return 0
        return None

    team = int(m.group(1))
    return team


def count_connections(conntract_reader):
    connect_cnt = collections.Counter()

    for line in conntract_reader:
        line = line.strip()

        m = LINE_RE.match(line)
        if not m:
            print("Bad line:", line, file=sys.stderr)
            continue

        proto, data_len, state, src_ip, dst_ip = m.groups()
        # udp connections don't have states
        if not state:
            state = "ESTABLISHED"
        data_len = int(data_len)

        src_team = ip_to_team(src_ip)
        dst_team = ip_to_team(dst_ip)

        # external connections are not interesting
        if src_team is None or dst_team is None:
            continue

        # skip not established connections
        if state != "ESTABLISHED":
            continue

        # udp and dccp are borring
        if proto in ["udp", "dccp"]:
            continue

        connect_cnt[(src_team, dst_team)] += 1
    return connect_cnt


def count_connections_from_conntrack():
    p = subprocess.Popen(CONNTRACK_ARGS, encoding="utf8",
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    connect_cnt = count_connections(p.stdout)
    p.wait(timeout=CHILD_WAIT_TIMEOUT)

    return connect_cnt


def handle_client(cl):
    connect_cnt = count_connections_from_conntrack()

    pkt_body_list = []
    pkt_body_list.append("# HELP ctf_connects The number of established connections between teams")
    pkt_body_list.append("# TYPE ctf_connects gauge")

    for src_team in range(TEAMS_NUM):
        for dst_team in range(TEAMS_NUM):
            val = connect_cnt[(src_team, dst_team)]

            src_team_metric_name = 'src_team="%d %s"' % (src_team, TEAM_NAMES[src_team])
            dst_team_metric_name = 'dst_team="%d %s"' % (dst_team, TEAM_NAMES[dst_team])

            metric = 'ctf_connects{%s,%s} %d' % (src_team_metric_name, dst_team_metric_name, val)
            pkt_body_list.append(metric)
    pkt_body = "\n".join(pkt_body_list) + "\n"

    pkt_header_list = []
    pkt_header_list.append("HTTP/1.1 200 OK")
    pkt_header_list.append("Content-Length: %d" % len(pkt_body))
    pkt_header_list.append("Content-Type: text/plain; version=0.0.4; charset=utf-8")
    pkt_header_list.append("Date: %s" % time.strftime("%a, %d %b %Y %H:%M:%S GMT", time.gmtime()))

    pkt_header = "\r\n".join(pkt_header_list)

    pkt = pkt_header + "\r\n\r\n" + pkt_body

    cl.sendall(pkt.encode("utf8"))
    cl.shutdown(socket.SHUT_RDWR)


def serve():
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", 9300))
    s.listen()

    while True:
        cl, cl_addr = s.accept()
        try:
            cl.settimeout(CLIENT_TIMEOUT)
            cl_host, cl_port = cl_addr

            client_good = False
            for prefix in JURY_NET_PREFIXES:
                if cl_host.startswith(prefix):
                    client_good = True

            if not client_good:
                continue

            handle_client(cl)

        except Exception as E:
            print("Exception while handling client", E, file=sys.stderr)
            traceback.print_exc()
        finally:
            cl.close()


def print_team_info(team):
    connect_cnt = count_connections_from_conntrack()

    print("Team %d (%s) outgoing connections:" % (team, TEAM_NAMES[team]))
    for dst_team in range(TEAMS_NUM):
        val = connect_cnt[(team, dst_team)]
        dst_team_name = TEAM_NAMES[dst_team]
        print("to team %d %s: %d" % (dst_team, dst_team_name, val))
    print()

    print("Team %d (%s) incoming connections:" % (team, TEAM_NAMES[team]))
    for src_team in range(TEAMS_NUM):
        val = connect_cnt[(src_team, team)]

        src_team_name = TEAM_NAMES[src_team]
        print("from team %d %s: %d" % (src_team, src_team_name, val))


if __name__ == "__main__":
    if len(sys.argv) <= 1:
        serve()
    else:
        print_team_info(int(sys.argv[1]))
