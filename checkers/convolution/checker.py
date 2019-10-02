#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import os
import requests
import json
import random
import string
import io
import json
import M2Crypto
import base64

# libssl-dev, swig
# m2crypto

SERVICE_NAME = "convolution"
OK, CORRUPT, MUMBLE, DOWN, CHECKER_ERROR = 101, 102, 103, 104, 110


def close(code, public="", private=""):
	if public:
		print(public)
	if private:
		print(private, file=stderr)
	print('Exit with code %d' % code, file=stderr)
	exit(code)


def check(*args):
	addr = args[0]

	# TODO process image and check
	
	close(OK)


def put(*args):
	addr = args[0]
	flag_id = args[1]
	flag = args[2]

	key = M2Crypto.EVP.load_key('privkey.pem')
	key.reset_context(md='sha256')
	key.sign_init()
	key.sign_update(flag.encode('utf-8'))
	signature = key.sign_final()
	signatureBase64 = base64.b64encode(signature)

	url = 'http://%s:8081/add-kernel?kernel-id=%s&kernel=%s' % (addr, flag_id, flag)
	try:
		r = requests.post(url, headers={'signature': signatureBase64})
		if r.status_code == 400 or r.status_code == 404:
			close(DOWN, "Service is down, status code=%u" % r.status_code)
		if r.status_code != 200:
			close(CORRUPT, "Service is corrupted, status code=%u" % r.status_code)

	except Exception as e:
		 close(DOWN, "HTTP Error", "HTTP error: %s" % e)

	close(OK, flag_id)


def get(*args):
	addr = args[0]
	flag_id = args[1]
	flag = args[2]

	url = 'http://%s:8081/list' % (addr)
	try:
		r = requests.get(url)
	except Exception as e:
		 close(DOWN, "HTTP Error", "HTTP error: %s" % e)

	if r.status_code == 400 or r.status_code == 404:
		close(DOWN, "Service is down, status code=%u" % r.status_code)
	if r.status_code != 200:
		close(CORRUPT, "Status code=%u" % r.status_code)		

	if r.text.find(flag_id) == -1:
		close(CORRUPT, "There is no kernel with id '%s'" % flag_id)

	url = 'http://%s:8081/get-kernel?kernel-id=%s' % (addr, flag_id)
	try:
		r = requests.get(url)
		if r.status_code == 400 or r.status_code == 404:
			close(DOWN, "Service is down, status code=%u" % r.status_code)
		if r.status_code != 200:
			close(CORRUPT, "Service is corrupted, status code=%u" % r.status_code)
	except Exception as e:
		 close(DOWN, "HTTP Error", "HTTP error: %s" % e)

	if r.text != flag:
		close(CORRUPT, "Service has returned wrong kernel", "Expected '%s', returned '%s'" % (flag, r.text))

	close(OK)


def info(*args):
    close(OK, "vulns: 1")


COMMANDS = {'check': check, 'put': put, 'get': get, 'info': info}


def not_found(*args):
    print("Unsupported command %s" % argv[1], file=stderr)
    return CHECKER_ERROR


if __name__ == '__main__':
	try:
		COMMANDS.get(argv[1], not_found)(*argv[2:])
	except Exception as e:
		close(CHECKER_ERROR, "Evil checker", "INTERNAL ERROR: %s" % e)
