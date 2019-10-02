#!/usr/bin/env python3

import sys
import os
import random
import string
import io
import json
import lxml.html
import requests

from urllib.parse import urlencode
from traceback import print_exc
from hashlib import sha256
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from base64 import b64encode, b64decode
from requests.packages.urllib3.exceptions import InsecureRequestWarning

SERVICE_NAME = 'handy'
OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

# How frequent new masters are created: on average every half of this number of rounds.
MASTER_SCARCITY_FACTOR = 20
PASSWORD_HASH_SALT = 'PLFRQPMMVACNIOUOKEOUNFBZUWUBMBQUYOLVNIUBAIIMSFEAZRAZZSFQZZRJWCGB'
USERNAME_MIN_LEN = 10
USERNAME_MAX_LEN = 15

PRIVATE_KEY_PATH = 'handy.private_key'
PRIVATE_KEY = None

def LoadPrivateKey(path):
    with open(path, "rb") as key_file:
        return serialization.load_pem_private_key(
            key_file.read(),
            password=None,
            backend=default_backend()
        )


# TODO: Something more clever
def GenerateTaskTitle():
    return 'A Task'

# TODO: Something more clever
def GenerateTaskDescription():
    return 'Some information'


def GeneratePassword(username):
    return sha256((username + PASSWORD_HASH_SALT).encode('utf-8')).hexdigest()


def GenerateUsername():
    username_len = random.randint(USERNAME_MIN_LEN, USERNAME_MAX_LEN)
    return ''.join(random.choice(string.ascii_lowercase) for i in range(username_len))


class HandyApi:
    def __init__(self, addr):
        self.addr = addr
        self.session = requests.Session()

    def Register(self, master=False):
        """Register a new user.

        Parameters:
            master -- whether the new user is a master.

        Returns:
            username
        """
        code = requests.codes.conflict
        while code == requests.codes.conflict:
            username = GenerateUsername()
            html, code = self._LoadUrl('register')
            Assert(code == requests.codes.ok, 'GET /register returned %d' % code)
            data = {
                'Username': username,
                'Password': GeneratePassword(username),
                'IsMaster': master,
            }
            if master:
                token = self._GetMasterToken(html)
                data.update({
                    'Token': token,
                    'SignedToken': self._SignMasterToken(token),
                })
            _, code = self._LoadUrl('register', data=data)
        Assert(code == requests.codes.ok, 'POST /register returned %d' % code)
        return username


    def Login(self, username):
        """Logins a user and saves a session."""
        data = {
            'Username': username,
            'Password': GeneratePassword(username),
        }
        _, code = self._LoadUrl('login', data=data)
        Assert(code == requests.codes.ok, 'POST /login returned %d' % code)

    def AddTask(self, payload):
        """Creates new task for some master."""
        # TODO: Only use checker-created masters
        available_masters = self._GetMasters()
        if len(available_masters) == 0:
            new_api = HandyApi(self.addr)
            new_api.Register(master=True)
            available_masters = self._GetMasters()
        if len(available_masters) == 0:
            close(MUMBLE, private='After master creation, none available')

        master_info = random.choice(available_masters)
        data = {
            'MasterId': master_info['id'],
            'Title': GenerateTaskTitle(),
            'Description': payload,
        }
        _, code = self._LoadUrl('tasks', data=data)
        Assert(code == requests.codes.ok, 'POST /tasks returned %d' % code)


    def GetTasks(self):
        """Returns all tasks for a given user.

        User should be logged-in.

        Returns:
            [tasks_from, tasks_to]
        """
        html_string, code = self._LoadUrl('tasks')
        Assert(code == requests.codes.ok, 'tasks returned %d' % code)
        try:
            html = lxml.html.fromstring(html_string)
            result = (
                self._ParseTasks(html, 'tasks-from-table'),
                self._ParseTasks(html, 'tasks-to-table'),
            )
            return result
        except Exception as e:
            close(MUMBLE, private='Failed to parse get tasks response: %s' % e)

    def GetProfile(self, id):
        """Return profile info for an ID.

        Returns:
            username -- username corresponding to ID
        """
        html_string, code = self._LoadUrl('profile?id=%s' % id)
        Assert(code == requests.codes.ok, 'GET /profile returned %d' % code)
        try:
            html = lxml.html.fromstring(html_string)
            for img in html.cssselect('img'):
                _, code = self._LoadUrl(img.get('src').lstrip('/'))
                Assert(code == requests.codes.ok, '(loading images) GET %s returned %d' % (img.get('src'), code))

            return html.cssselect('#username')[0].text_content()
        except Exception as e:
            close(MUMBLE, private='Failed to parse profile response: %s' % e)

    def _ParseTasks(self, html, table_class):
        trs = html.cssselect('.%s tbody tr' % table_class)
        result = []
        for tr in trs:
            title_element, profile_element, description_element = tr.cssselect('td')
            result.append({
                'title': title_element.text_content(),
                'description': description_element.text_content(),
                'id': profile_element[0].get('href')[-36:],
            })
        return result

    def _LoadUrl(self, relative_url, data=None):
        url = 'https://%s/%s' % (self.addr, relative_url)
        try:
            if data is None:
                res = self.session.get(url, verify=False)
            else:
                res = self.session.post(url, data=data, verify=False)
            return res.text, res.status_code
        except requests.ConnectionError as e:
            close(DOWN, private='Failed to communicate: %s' % e)
        except Exception as e:
            close(MUMBLE, private='Failed to communicate: %s' % e)

    def _GetMasters(self):
        html_string, code = self._LoadUrl('masters')
        Assert(code == requests.codes.ok, 'GET /masters returned %d' % code)
        try:
            html = lxml.html.fromstring(html_string)
            links = html.cssselect('.masters-table a')
            return [ { 'id': link.get('href')[-36:], } for link in links ]
        except Exception as e:
            close(MUMBLE, private='Failed to parse get masters response: %s' % e)

    def _GetMasterToken(self, html_string):
        try:
            html = lxml.html.fromstring(html_string)
            token_inputs = html.cssselect('#token')
            Assert(len(token_inputs) == 1, 'Master form contains none or several token inputs')
            token = token_inputs[0].get('value')
            Assert(token is not None, 'No token found in the register HTML')
            return token
        except Exception as e:
            close(MUMBLE, private='Failed to parse out master token: %s' % e)

    def _SignMasterToken(self, token):
        try:
            token_bytes = b64decode(token)
            signature_bytes = PRIVATE_KEY.sign(
                token_bytes,
                padding.PSS(
                    mgf=padding.MGF1(hashes.SHA256()),
                    salt_length=padding.PSS.MAX_LENGTH
                ),
                hashes.SHA256()
            )
            return b64encode(signature_bytes)
        except Exception as e:
            close(MUMBLE, private='Failed to sign master token: %s' % e)


def VerifyTaskExists(addr, username, payload):
    user_api = HandyApi(addr)
    user_api.Login(username)
    tasks_from, _ = user_api.GetTasks()
    tasks_with_payload = [ task for task in tasks_from if task['description'] == payload ]
    if len(tasks_with_payload) == 0:
        close(CORRUPT,
            public='Flag is missing',
            private='Flag %s is not present for user %s' % (payload, username))

    task = tasks_with_payload[0]
    master_username = user_api.GetProfile(task['id'])
    master_api = HandyApi(addr)
    master_api.Login(master_username)
    _, tasks_to = master_api.GetTasks()
    tasks_with_payload = [ task['description'] == payload for task in tasks_to ]
    if len(tasks_with_payload) == 0:
        close(CORRUPT,
            public='Flag is missing',
            private='Master doesn\'t contain flag %s for user %s' % (payload, username))


def close(code, public='', private=''):
    if public:
        print(public)
    if private:
        print(private, file=sys.stderr)
    print('Exit with code %d' % code, file=sys.stderr)
    exit(code)


def Assert(condition, msg):
    if condition:
        return
    close(MUMBLE, private='Assertion failed: %s' % msg)


def check(*args):
    addr = args[0]
    api = HandyApi(addr)

    if random.randint(0, MASTER_SCARCITY_FACTOR) == 0:
        api.Register(master=True)

    username = api.Register()
    api.Login(username)
    payload = GenerateTaskDescription()
    api.AddTask(payload)

    VerifyTaskExists(addr, username, payload)

    close(OK)


def put(*args):
    addr, flag = args[0], args[2]

    api = HandyApi(addr)
    username = api.Register()
    api.Login(username)
    api.AddTask(flag)

    close(OK, username)


def get(*args):
    addr, username, flag, _ = args

    VerifyTaskExists(addr, username, flag)

    close(OK)


def info(*args):
    close(OK, 'vulns: 1')


def not_found(*args):
    print('Unsupported command %s' % sys.argv[1], file=sys.stderr)
    return CHECKER_ERROR


COMMANDS = {'check': check, 'put': put, 'get': get, 'info': info}


if __name__ == '__main__':
    requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
    try:
        PRIVATE_KEY = LoadPrivateKey(PRIVATE_KEY_PATH)
        COMMANDS.get(sys.argv[1], not_found)(*sys.argv[2:])
    except Exception as e:
        print_exc(file=sys.stderr)
        close(CHECKER_ERROR, 'Problem with checker', 'INTERNAL ERROR: %s' % e)
