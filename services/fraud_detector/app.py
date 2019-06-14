import sys
import pathlib
import json

import fraud_detector

STATIC_DIR = pathlib.Path("static")
USERS_DIR = pathlib.Path("data/users/")
RULES_DIR = pathlib.Path("data/rules/")

DYNAMIC_HEADERS = [
    ("Content-Type", "application/json; charset=utf8"),
    ("Cache-Control", "no-cache, no-store, max-age=0, must-revalidate"),
    ("Expires", "0"),
    ("Pragma", "no-cache"),
]

JSON_HEADERS = [
    ("Content-Type", "application/json; charset=utf8"),
]

HTML_HEADERS = [
    ("Content-Type", "text/html; charset=utf8"),
]

IMG_HEADERS = [
    ("Content-Type", "image/png"),
    ("Cache-Control", "max-age=3600"),
]


def gen_json_ans(obj):
    return "200 OK", JSON_HEADERS, json.dumps(obj)


def get_users(body):
    users = json.load(open(USERS_DIR / "users.json"))
    return gen_json_ans(users)


def get_rules(body):
    rule_files = sorted(RULES_DIR.iterdir(), key=lambda f: -f.lstat().st_mtime)
    rules = [file.name for file in rule_files]
    return gen_json_ans(rules)


def add_rule(body):
    name = body.get("name")
    code = body.get("code")
    if type(code) is not str or not code:
        return gen_json_ans("error: no code")
    if len(code) > 2048:
        return gen_json_ans("error: the code is too long")
    try:
        compile(code, filename="rule.py", mode="exec")
    except Exception:
        return gen_json_ans("error: bad code")

    if type(name) is not str or not name:
        return gen_json_ans("error: no name")
    if len(name) > 64:
        return gen_json_ans("error: the name is too long")
    if "/" in name:
        return gen_json_ans("error: bad name")
    try:
        with open(RULES_DIR / name, "x") as file:
            file.write(code)
    except OSError:
        return gen_json_ans("error: bad name")

    return gen_json_ans("ok: rule has been added")


def check_user(body):
    user_index = body.get("user", -1)
    rules = body.get("rules", [])

    if type(user_index) is not int or user_index < 0:
        return gen_json_ans("error: no user")

    users = json.load(open(USERS_DIR / "users.json"))
    try:
        user = users[user_index]
    except LookupError:
        return gen_json_ans("error: bad user")

    if type(rules) is not list or not rules:
        return gen_json_ans("error: no rules")
    if len(rules) > 256:
        return gen_json_ans("error: too much rules")

    rules_contents = []
    for rule_name in rules:
        if type(rule_name) is not str or "/" in rule_name or len(rule_name) > 64:
            return gen_json_ans("error: bad rule")
        try:
            rule = (RULES_DIR / rule_name).read_text(encoding="utf8")
            rules_contents.append(rule)
        except OSError:
            return gen_json_ans("error: bad rule")

    try:
        ans = fraud_detector.run_rules_safe(rules_contents, user)
    except Exception as e:
        return gen_json_ans("error: %s" % type(e).__name__)
    return gen_json_ans(ans)


URLS = {
    ("GET", "/"): "index.html",
    ("GET", "/index.html"): "index.html",
    ("GET", "/favicon.ico"): "favicon.ico",
    ("GET", "/taxi.png"): "taxi.png",
    ("GET", "/noavatar.png"): "noavatar.png",
    ("GET", "/hacker.png"): "hacker.png",
    ("GET", "/rules"): get_rules,
    ("GET", "/users"): get_users,
    ("POST", "/addrule"): add_rule,
    ("POST", "/checkuser"): check_user,
}


def get_request_body_json(environ):
    try:
        request_body_size = int(environ.get('CONTENT_LENGTH', 0))
    except ValueError:
        request_body_size = 0
    request_body = environ['wsgi.input'].read(request_body_size)
    if not request_body:
        return ""
    return json.loads(request_body.decode())


def application(environ, start_response):
    method = environ["REQUEST_METHOD"]
    url = environ["PATH_INFO"]

    if (method, url) not in URLS:
        start_response("404 NOT FOUND", HTML_HEADERS)
        return [b"404"]

    handler = URLS[(method, url)]
    if not callable(handler):
        if url.endswith((".png", ".ico")):
            start_response("200 OK", IMG_HEADERS)
        else:
            start_response("200 OK", HTML_HEADERS)
        return [(STATIC_DIR / handler).read_bytes()]

    status, headers, data = handler(get_request_body_json(environ))
    start_response(status, headers)
    return [data.encode()]
