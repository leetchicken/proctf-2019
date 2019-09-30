#!/usr/bin/env python3

import sys
import hashlib
import random
import re
import functools
import time
import json
import pathlib
import base64
import traceback
import socket

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

PORT = 10001
TIMEOUT = 3

SCRIPT_PATH = pathlib.Path(__file__).parent

ABC = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789"


class P:
    def __init__(self, s):
        self.stdout = s.makefile("r")
        self.stdin = s.makefile("w")


def get_logins(p):
    logins = []
    avail_logins_prompt = p.stdout.readline()
    while True:
        buf = p.stdout.readline().strip()
        if "Please log in or register" in buf:
            break
        logins.append(buf)
    return [l for l in logins if "hack" not in l]


def draw_line_low(draw_buf, x1, y1, x2, y2):
    dx = x2 - x1
    dy = abs(y2 - y1)
    sy = -1 if (y2 - y1) < 0 else 1
    D = 2*dy - dx
    y = y1

    for x in range(x1, x2+1):
        if y >= 0 and y < 25 and x >= 0 and x < 80:
            draw_buf[y][x] = 1

        if D > 0:
            y = y + sy
            D = D - 2*dx
        D = D + 2*dy


def draw_line_high(draw_buf, x1, y1, x2, y2):
    dx = abs(x2 - x1)
    dy = y2 - y1
    sx = -1 if (x2 - x1) < 0 else 1
    D = 2*dx - dy
    x = x1

    for y in range(y1, y2+1):
        if y >= 0 and y < 25 and x >= 0 and x < 80:
            draw_buf[y][x] = 1

        if D > 0:
            x = x + sx
            D = D - 2*dy
        D = D + 2*dx


def draw_line(draw_buf, x1, y1, x2, y2):
    if abs(y2 - y1) < abs(x2 - x1):
        if x1 > x2:
            draw_line_low(draw_buf, x2, y2, x1, y1)
        else:
            draw_line_low(draw_buf, x1, y1, x2, y2)
    else:
        if y1 > y2:
            draw_line_high(draw_buf, x2, y2, x1, y1)
        else:
            draw_line_high(draw_buf, x1, y1, x2, y2)


def draw_poly(points, draw_buf):
    if not (points[0][0] == 0 and points[0][1] == 0):
        for l in range(len(points)-1):
            draw_line(draw_buf, points[l][0], points[l][1], points[l+1][0], points[l+1][1])
        draw_line(draw_buf, points[-1][0], points[-1][1], points[0][0], points[0][1])

    while(1):
        replace_made = 0
        for y in range(25):
            for x in range(80):
                if draw_buf[y][x] != 0:
                    continue

                if x == 0 or draw_buf[y][x-1] == 2:
                    draw_buf[y][x] = 2
                    replace_made = 1
                if x == 79 or draw_buf[y][x+1] == 2:
                    draw_buf[y][x] = 2
                    replace_made = 1
                if y == 0 or draw_buf[y-1][x] == 2:
                    draw_buf[y][x] = 2
                    replace_made = 1
                if y == 24 or draw_buf[y+1][x] == 2:
                    draw_buf[y][x] = 2
                    replace_made = 1
        if replace_made == 0:
            break

    for y in range(25):
        for x in range(80):
            draw_buf[y][x] = int(draw_buf[y][x] == 0)


def render(polys):
    rendered = [[0] * 80 for i in range(25)]

    for poly in range(len(polys)):
        draw_buf = [[0] * 80 for i in range(25)]
        draw_poly(polys[poly], draw_buf)

        for y in range(0, 25):
            for x in range(0, 80):
                if draw_buf[y][x] != 0:
                    rendered[y][x] = chr(ord('A') + poly)

    output = []
    for y in range(0, 25):
        for x in range(0, 80):
            if rendered[y][x] == 0:
                output.append(" ")
            else:
                output.append(rendered[y][x])
        output.append("\n")
    return "".join(output)


def cmd(p, text, expected_output=1):
    if not text.endswith("\n"):
        text += "\n"
    p.stdin.write(text)
    p.stdin.flush()
    ans = []
    for i in range(expected_output):
        ans.append(p.stdout.readline().rstrip("\n"))
    return "\n".join(ans)


def verdict(exit_code, public="", private=""):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    sys.exit(exit_code)


def info():
    verdict(OK, "vulns: 1")


def check(host):
    p = P(socket.create_connection((host, PORT), timeout=TIMEOUT))

    welcome_prompt = p.stdout.readline()
    get_logins(p)

    login = "".join(random.choice(ABC) for i in range(random.randrange(8, 14)))
    password = "".join(random.choice(ABC) for i in range(random.randrange(8, 14)))
    flag = "".join(random.choice(ABC) for i in range(random.randrange(8, 14)))

    p.stdin.write(login + "\n")
    p.stdin.write(password + "\n")
    p.stdin.write(flag + "\n")
    p.stdin.flush()

    # check 1: the memory control logic is not removed
    ans = cmd(p, "new_poly\n"*25 + "set_frame 1\n" +
                 "new_poly\n"*15 + "set_frame 2\n" +
                 "new_poly\n"*9, expected_output=25+1+15+1+9)

    ans = cmd(p, "new_poly")
    if "ok" not in ans:
        verdict(MUMBLE, "Check failed: adding new poly failed",
                "Check failed on stage 1, adding many polys: %s" % ans)

    ans = cmd(p, "new_poly")
    if "too much polys" not in ans:
        verdict(MUMBLE, "Check failed: some memory control logic was deleted",
                "Check failed on stage 1, the memory limit deleted: %s" % ans)

    ans = cmd(p, "del_poly 0")
    if "ok" not in ans:
        verdict(MUMBLE, "Check failed: deleting poly failed",
                "Check failed on stage 1, deleting poly failed: %s" % ans)

    ans = cmd(p, "new_poly")
    if "ok" not in ans:
        verdict(MUMBLE, "Check failed: adding a poly failed",
                "Check failed on stage 1, the free logic is likely removed: %s" % ans)

    # check 2: render
    polys = []
    for poly_pos in range(random.randrange(2, 10)):
        points = []
        for i in range(random.randrange(5, 30)):
            points.append((random.randrange(1, 80), random.randrange(1, 25)))
        polys.append(points)

    for poly_pos, points in enumerate(polys):
        add_points_cmd = ""
        for point_pos, point in enumerate(points):
            add_points_cmd += "add_point %d %d %d %d\n" % (poly_pos, point_pos, point[0], point[1])
        ans = cmd(p, add_points_cmd, expected_output=len(points))
        if ans.count("ok") != len(points):
            verdict(MUMBLE, "Check failed: adding a point failed",
                    "Check failed on stage 2, adding a points failed: not all answers are true")

    # checking 3 random points
    for i in range(3):
        poly_pos = random.randrange(len(polys))
        points = polys[poly_pos]
        point_pos = random.randrange(len(points))

        ans = cmd(p, "get_point %d %d" % (poly_pos, point_pos))
        if "x=%d" % points[point_pos][0] not in ans or "y=%d" % points[point_pos][1] not in ans:
            verdict(MUMBLE, "Check failed: getting a point failed",
                    "Check failed on stage 2, getting a point failed ans=%s" % ans)

    ans = cmd(p, "render", expected_output=25+1)
    try:
        first_line, ans = ans.split("\n", 1)
    except Exception:
        verdict(MUMBLE, "Render failed",
                "Check failed on stage 2, wrong render answer")

    if flag not in first_line:
        verdict(MUMBLE, "Check failed: getting a secret failed",
                "Check failed on stage 2, secret not in render answer")

    if render(polys).strip("\n") != ans.strip("\n"):
        verdict(MUMBLE, "Check failed: render result is unexpected",
                "Check failed on stage 2, render result is unexpected")

    # check 3: copy operations
    poly_idx = random.randrange(len(polys))
    cmd(p, "copy_poly %d 9" % poly_idx)
    cmd(p, "del_point %d %d" % (poly_idx, len(polys[poly_idx]) - 1))

    last_point = (random.randrange(200, 256), random.randrange(200, 256))
    cmd(p, "add_point %d %d %d %d" % (poly_idx, len(polys[poly_idx]) - 2,
        last_point[0], last_point[1]))
    cmd(p, "set_frame 9")

    ans = cmd(p, "get_point 0 %d" % (len(polys[poly_idx]) - 1))
    if "x=" in ans:
        verdict(MUMBLE, "Check failed: copying or deleting point doesnt work",
                "Check failed on stage 3, copy or delete test fail: %s" % ans)

    ans = cmd(p, "get_point 0 %d" % (len(polys[poly_idx]) - 2))
    if "x=%d" % (last_point[0]) not in ans or "y=%d" % (last_point[1]) not in ans:
        verdict(MUMBLE, "Check failed: copying or deleting point doesnt work",
                "Check failed on stage 3, copy or delete test 2 fail: %s" % ans)

    cmd(p, "copy_frame 8\nclear_frame\n", expected_output=3)
    ans = cmd(p, "get_point 0 %d" % (len(polys[poly_idx]) - 2))
    if "x=" in ans:
        verdict(MUMBLE, "Check failed: clear frame doesnt work",
                "Check failed on stage 3, clear frame test fail: %s" % ans)
    cmd(p, "set_frame 8")
    ans = cmd(p, "get_point 0 %d" % (len(polys[poly_idx]) - 2))
    if "x=%d" % last_point[0] not in ans or "y=%d" % last_point[1] not in ans:
        verdict(MUMBLE, "Check failed: frame copy doesnt work",
                "Check failed on stage 3, frame copy test fail: %s" % ans)
    verdict(OK)


def put(host, flag_id, flag, vuln):
    p = P(socket.create_connection((host, PORT), timeout=TIMEOUT))

    welcome_prompt = p.stdout.readline()
    get_logins(p)

    login = "".join(random.choice(ABC) for i in range(random.randrange(8, 14)))
    password = "".join(random.choice(ABC) for i in range(random.randrange(8, 14)))

    p.stdin.write(login + "\n")
    p.stdin.write(password + "\n")
    p.stdin.write(flag + "\n")
    p.stdin.flush()

    ans = cmd(p, "new_poly")
    if "new secret" not in ans or "ok, idx=0" not in ans:
        verdict(MUMBLE, "bad answer message while putting the flag")

    flag_id = base64.b64encode(json.dumps([login, password]).encode()).decode()
    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    p = P(socket.create_connection((host, 10001), timeout=TIMEOUT))

    welcome_prompt = p.stdout.readline()
    logins = get_logins(p)

    try:
        login, password = json.loads(base64.b64decode(flag_id))
    except Exception:
        verdict(MUMBLE, "Bad flag id", "Bad flag_id: %s" % traceback.format_exc())

    if login not in logins:
        verdict(MUMBLE, "User list error", "User list error: no login %s" % login)

    p.stdin.write(login + "\n")
    p.stdin.write(password + "\n")
    p.stdin.flush()

    ans = cmd(p, "new_poly")

    if "ok, idx=0" not in ans:
        verdict(MUMBLE, "no such flag",
                        "bad answer message while getting the flag: %s" % ans)

    ans = cmd(p, "render", expected_output=25+1)
    if flag not in ans:
        verdict(MUMBLE, "bad flag", "bad flag: %s" % ans)
    verdict(OK)


def main(args):
    CMD_MAPPING = {
        "info": (info, 0),
        "check": (check, 1),
        "put": (put, 4),
        "get": (get, 4),
    }

    if not args:
        verdict(CHECKER_ERROR, "No args", "No args")

    cmd, args = args[0], args[1:]
    if cmd not in CMD_MAPPING:
        verdict(CHECKER_ERROR, "Checker error", "Wrong command %s" % cmd)

    handler, args_count = CMD_MAPPING[cmd]
    if len(args) != args_count:
        verdict(CHECKER_ERROR, "Checker error", "Wrong args count for %s" % cmd)

    try:
        handler(*args)
    except ConnectionError as E:
        verdict(DOWN, "Connect error", "Connect error: %s" % E)
    except socket.timeout as E:
        verdict(DOWN, "Timeout", "Timeout: %s" % E)
    except json.decoder.JSONDecodeError as E:
        verdict(MUMBLE, "Json decode error", "Json decode error: %s" % traceback.format_exc())
    except Exception as E:
        verdict(CHECKER_ERROR, "Checker error", "Checker error: %s" % traceback.format_exc())
    verdict(CHECKER_ERROR, "Checker error", "No verdict")


if __name__ == "__main__":
    main(args=sys.argv[1:])
