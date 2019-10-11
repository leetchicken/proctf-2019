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
import requests

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.serialization import load_der_private_key

from user_agents import USER_AGENTS

OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

UPDATER_PORT = 3255
HTTP_PORT = 80
TIMEOUT = 5

PRIVATE_KEY = ("302e020100300506032b657004220420cfbff62701f76c39" +
               "51f487928e6cea1d2d66164ce9f0a3e8d27d8e9944c028eb")

SCRIPT_PATH = pathlib.Path(__file__).parent
ABC = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

HOSTS = json.load(open("hosts.json"))

HOSTS_HTTP = HOSTS["http"]
HOSTS_HTTP2 = HOSTS["http2"]


class P:
    def __init__(self, s):
        self.stdout = s.makefile("r")
        self.stdin = s.makefile("w")


def create_session():
    s = requests.Session()

    # add timeouts
    s.get = functools.partial(s.get, timeout=TIMEOUT)
    s.post = functools.partial(s.post, timeout=TIMEOUT)
    s.headers["User-Agent"] = random.choice(USER_AGENTS)
    return s


def gen_secret_string():
    return "".join(random.choice(ABC) for i in range(random.randrange(8, 12)))


def sign(data):
    signer = load_der_private_key(bytes.fromhex(PRIVATE_KEY), password=None,
                                  backend=default_backend())
    return signer.sign(data.encode()).hex()


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


def calc_checksum(data):
    q = hashlib.sha256(data).hexdigest()

    for c in "HASH_SUM_IS_STRONG":
        q = hashlib.sha256((q + c).encode()).hexdigest()

    return q[:32]


def get_team_by_ip(ip):
    m = re.match(r"\d+\.\d+\.(\d+)\.\d+", ip)
    if m:
        return int(m.group(1))
    return None


def call_update(p, urls, fake_urls=None, team=None):
    if fake_urls is None:
        fake_urls = []

    urls = urls[:]
    random.shuffle(urls)

    fake_urls = fake_urls[:]
    random.shuffle(fake_urls)

    prompt1 = p.stdout.readline().strip()
    if not prompt1.startswith("Upgrade service ready"):
        verdict(MUMBLE, "bad welcome prompt", "bad weclome prompt1: %s" % prompt1)
    prompt2 = p.stdout.readline().strip()
    if not prompt2.startswith("Enter address"):
        verdict(MUMBLE, "bad welcome prompt", "bad welcome prompt2: %s" % prompt2)

    protocol = random.choice(["http", "http2"])

    while True:
        if protocol == "http":
            update_host = random.choice(HOSTS_HTTP)
        else:
            update_host = random.choice(HOSTS_HTTP2)

        # do not ask a host from the same network
        if team and get_team_by_ip(update_host) == team:
            continue

        break

    p.stdin.write("%s\n" % update_host)
    p.stdin.flush()

    prompt3 = p.stdout.readline().strip()
    if not prompt3.startswith("Enter manifest"):
        verdict(MUMBLE, "bad manifest prompt", "bad manifest prompt: %s" % prompt3)


    links = []

    for url in fake_urls:
        link = {}
        link["url"] = "%s://mirror/%s" % (protocol, url)
        link["checksum"] = calc_checksum(gen_secret_string().encode())
        links.append(link)

    for url in urls:
        link = {}
        link["url"] = "%s://mirror/%s" % (protocol, url)
        try:
            checksum = calc_checksum(open(SCRIPT_PATH / "srv" / "site" / url, "rb").read())
        except OSError as E:
            verdict(CHECKER_ERROR, "file open error", "file open error: %s" % E)

        link["checksum"] = checksum

        links.append(link)

    manifest = json.dumps({"links": links})

    p.stdin.write(manifest+"\n")
    p.stdin.flush()

    prompt4 = p.stdout.readline().strip()
    if not prompt4.startswith("Enter manifest signature"):
        verdict(MUMBLE, "bad manifest signature prompt",
                "bad manifest signature prompt: %s" % prompt4)

    signature = sign(manifest)

    #print(manifest, signature, file=sys.stderr)

    p.stdin.write(signature+"\n")
    p.stdin.flush()

    sig_check_result = p.stdout.readline().strip()
    if not sig_check_result.startswith("Signature is ok"):
        verdict(MUMBLE, "server doesn't accept signed data",
                "bad manifest signature: %s" % sig_check_result)

    for pos, link in enumerate(links):
        link_result = p.stdout.readline().strip()
        if pos < len(fake_urls):
            continue
        if " OK" not in link_result:
            verdict(MUMBLE, "bad download status",
                    "bad download status: %s, ans is: %s" % (link, link_result))
        # print("result", link_result)


def get_urls():
    try:
        return json.load(open(SCRIPT_PATH / "sitemap.json"))
    except Exception as E:
        verdict(CHECKER_ERROR, "failed to load sitemap",
                "failed to load sitemap: %s" % E)


def check(host):
    try:
        p = P(socket.create_connection((host, UPDATER_PORT), timeout=TIMEOUT))
    except Exception as E:
        verdict(DOWN, "connect failed", "connect failed: %s" % E)

    urls = get_urls()
    fake_urls = [random.choice(urls["grb"])["name"]]

    filenames = [u["name"] for u in random.sample(urls["grb"], 2) + [random.choice(urls["idx"])]]
    call_update(p, filenames, fake_urls, team=get_team_by_ip(host))

    s = create_session()

    for filename in filenames:
        try:
            file_data = open(SCRIPT_PATH / "srv" / "site" / filename, "r").read()
        except Exception as E:
            verdict(CHECKER_ERROR, "failed to load file to compare with",
                    "failed to load file to compare with: %s" % E)

        if "/" in filename:
            filename = filename.split("/")[-1]

        url = "http://%s:%d/%s" % (host, HTTP_PORT, filename)
        try:
            resp = s.get(url).text
        except Exception as E:
            verdict(DOWN, "failed to get http answer", "%s" % E)

        if file_data != resp and filename != "index.html":
            verdict(MUMBLE, "update failed", "bad update data: %s" % file_data)

    verdict(OK)


def put(host, flag_id, flag, vuln):
    try:
        p = P(socket.create_connection((host, UPDATER_PORT), timeout=TIMEOUT))
    except Exception as E:
        verdict(DOWN, "connect failed", "connect failed: %s" % E)

    urls = get_urls()

    putter = random.choice(urls["t1"])
    lister_name = random.choice(urls["t2"])["name"]

    call_update(p, [putter["name"], lister_name], team=get_team_by_ip(host))

    prefix = putter["prefix"]

    s = create_session()
    url = "http://%s:%d/%s" % (host, HTTP_PORT, putter["name"])

    flag_id = random.randrange(10**12)
    key = gen_secret_string()
    params = {"id": flag_id, "key": key, "flag": flag}

    try:
        resp = s.post(url, data=params).text
        if "OK" not in resp:
            verdict(MUMBLE, "put failed", "put key failed, ans=%s" % resp)
    except Exception as E:
        verdict(MUMBLE, "put failed", "put key failed: " % E)

    check_file = prefix + "_" + str(flag_id) + ".php"

    flag_id = base64.b64encode(json.dumps([check_file, key, lister_name]).encode()).decode()
    verdict(OK, flag_id)


def get(host, flag_id, flag, vuln):
    try:
        check_file, key, lister_name = json.loads(base64.b64decode(flag_id))
    except Exception:
        verdict(CHECKER_ERROR, "Bad flag id", "Bad flag_id: %s" % traceback.format_exc())

    s = create_session()

    try:
        url = "http://%s:%d/%s" % (host, HTTP_PORT, lister_name)
        ans = s.get(url).text
        if check_file not in ans:
            verdict(MUMBLE, "file listing failed", "file listing failed, no checkfile")
    except Exception as E:
        verdict(MUMBLE, "file listing failed", "file listing failed: " % E)

    try:
        url = "http://%s:%d/%s" % (host, HTTP_PORT, check_file)
        ans = s.get(url, params={"key": key}).text
        if flag not in ans:
            verdict(MUMBLE, "no such flag", "no such flag, check_file %s key %s" %
                    (check_file, key))
    except Exception as E:
        verdict(MUMBLE, "file listing failed", "file listing failed: " % E)

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
