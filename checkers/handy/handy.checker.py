#!/usr/bin/env python3

import sys
import os
import requests
import random
import string
import io
import json

from urllib.parse import urlencode
from traceback import print_exc
from hashlib import sha256

import requests

SERVICE_NAME = 'handy'
OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110

# How frequent new masters are created: on average every half of this number of rounds.
MASTER_SCARCITY_FACTOR = 20
PASSWORD_HASH_SALT = 'PLFRQPMMVACNIOUOKEOUNFBZUWUBMBQUYOLVNIUBAIIMSFEAZRAZZSFQZZRJWCGB'
USERNAME_MIN_LEN = 10
USERNAME_MAX_LEN = 15


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
			data = {
				'Username': username,
				'Password': GeneratePassword(username),
				'IsMaster': master,
			}
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
		available_masters = self._GetMasters()
		if len(available_masters) == 0:
			self.Register(master=True)
			available_masters = self._GetMasters()
		if len(available_masters) == 0:
			close(MUMBLE, private='After master creation, none available')

		master_info = random.choice(available_masters)
		data = {
			'MasterId': master_info['id'],
			'description': payload,
		}
		_, code = self._LoadUrl('tasks', data=data)
		Assert(code == requests.codes.ok, 'POST /tasks returned %d' % code)


	def GetTasks(self):
		"""Returns all tasks for a given user.

		User should be logged-in.

		Returns:
			[tasks_from, tasks_to]
		"""
		tasks_encoded, code = self._LoadUrl('tasks')
		Assert(code == requests.codes.ok, 'tasks returned %d' % code)
		try:
			tasks = json.loads(tasks_encoded)
		except Exception as e:
			close(MUMBLE, private='Failed to parse get tasks response: %s' % e)
		Assert(isinstance(tasks, dict), 'GET /tasks returned %s, which is not a dict' % tasks_encoded)
		return [
			tasks['tasks_from'] if 'tasks_from' in tasks else [],
			tasks['tasks_to'] if 'tasks_to' in tasks else [],
		]

	def GetProfile(self, id):
		"""Return profile info for an ID.

		Returns:
			username -- username corresponding to ID
		"""
		# TODO: load img
		profile_encoded, code = self._LoadUrl('profile?id=%s' % id)
		Assert(code == requests.codes.ok, 'GET /profile returned %d' % code)
		try:
			profile = json.loads(profile_encoded)
		except Exception as e:
			close(MUMBLE, private='Failed to parse profile response: %s' % e)
		Assert(isinstance(profile, dict), 'GET /profile returned %s, which is not a dict' % profile_encoded)
		Assert("username" in profile, 'GET /profile returned %s, which doesn\'t have username' % profile_encoded)
		return profile["username"]

	def _LoadUrl(self, relative_url, data=None):
		url = 'http://%s/%s' % (self.addr, relative_url)
		try:
			if data is None:
				res = self.session.get(url)
			else:
				res = self.session.post(url, data=data)
			return res.text, res.status_code
		except requests.ConnectionError as e:
			close(DOWN, private='Failed to communicate: %s' % e)
		except Exception as e:
			close(MUMBLE, private='Failed to communicate: %s' % e)

	def _GetMasters(self):
		available_masters_str, code = self._LoadUrl('masters')
		Assert(code == requests.codes.ok, 'GET /masters returned %d' % code)
		try:
			return json.loads(available_masters_str)
		except Exception as e:
			close(MUMBLE, private='Failed to parse get masters response: %s' % e)


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
	master_username = user_api.GetProfile(task['master_id'])
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
	# TODO: replace with proper text generation.
	payload = 'sum info'
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
	try:
		COMMANDS.get(sys.argv[1], not_found)(*sys.argv[2:])
	except Exception as e:
		print_exc(file=sys.stderr)
		close(CHECKER_ERROR, 'Problem with checker', 'INTERNAL ERROR: %s' % e)
