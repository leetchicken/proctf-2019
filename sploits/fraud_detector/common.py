import requests
import json


PORT = 10000


def call_get_rules_api(host):
    url = "http://%s:%d/rules" % (host, PORT)
    ans = requests.get(url)
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not list or any(type(o) != str for o in ans_obj):
        return None
    return ans_obj


def call_add_rule_api(host, name, code):
    url = "http://%s:%d/addrule" % (host, PORT)
    ans = requests.post(url, data=json.dumps({"name": name, "code": code}))
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not str:
        return None
    return ans_obj


def call_check_user_api(host, rules, user):
    url = "http://%s:%d/checkuser" % (host, PORT)
    ans = requests.post(url, data=json.dumps({"rules": rules, "user": user}))
    if ans.status_code != 200:
        return None
    ans_obj = ans.json()
    if type(ans_obj) is not list or any(type(o) != int for o in ans_obj):
        return None
    return ans_obj
