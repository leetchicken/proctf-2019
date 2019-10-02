#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import requests
import random
import string
import io
import json
import M2Crypto
import base64
import UserAgents
from PIL import Image, ImageDraw

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

	url = 'http://%s:8081/list' % (addr)
	try:
		r = requests.get(url, headers={'User-Agent' : UserAgents.get()})
	except Exception as e:
		 close(DOWN, "HTTP Error", "HTTP error: %s" % e)

	if r.status_code == 404:
		close(DOWN, "Service is down, status code=%u" % r.status_code)
	if r.status_code != 200:
		close(CORRUPT, "Status code=%u" % r.status_code)

	if not r.text:
		close(OK)

	# choose random id
	lines = r.text.split('\n')
	id = lines[random.randint(0, len(lines) - 2)]

	# generate images
	imgStreams = {}
	for i in range(3):
		width = random.randint(32, 384)
		height = random.randint(32, 384)
		img = Image.new('RGBA', (width, height))
		draw = ImageDraw.Draw(img)
		for x in range(0, width):
			for y in range(0, height):
				r = random.randint(0, 255)
				g = random.randint(0, 255)
				b = random.randint(0, 255)
				a = random.randint(0, 255)
				draw.point((x,y), fill=(r,g,b,a))
		del draw
	
		name = ''.join(random.choice(string.ascii_uppercase + string.digits) for j in range(32))
		stream = io.BytesIO()
		img.save(stream, format="png")
		imgStreams.update({name : stream.getvalue()})

	# ask service to process generated images
	url = 'http://%s:8081/process?kernel-id=%s' % (addr, id)
	try:
		r = requests.post(url, headers={'User-Agent' : UserAgents.get()}, files=imgStreams)
		if r.status_code == 404:
			close(DOWN, "Service is down, status code=%u" % r.status_code)
		if r.status_code != 200:
			close(CORRUPT, "Service is corrupted, status code=%u" % r.status_code)

	except Exception as e:
		 close(DOWN, "HTTP Error", "HTTP error: %s" % e)

	# check response
	try:
		parsedRespone = json.loads(r.text)
		if len(parsedRespone) != len(imgStreams):
			close(CORRUPT, "Asked service to process %u images, but processed %u" % (len(imgStreams), len(parsedRespone)))
	except Exception as e:
		close(CORRUPT, "Invalid response", "Invalid response: %s" % e)

	for name, timings in parsedRespone.items():
		if not name in imgStreams:
			close(CORRUPT, "Unknown image name '%s'" % name)
		
		if len(timings) != 4:
			close(CORRUPT, "Wrong timings format %s" % timings)

		if not 'red_channel' in timings:
			close(CORRUPT, "Wrong timings format %s" % timings)
		if not 'green_channel' in timings:
			close(CORRUPT, "Wrong timings format %s" % timings)
		if not 'blue_channel' in timings:
			close(CORRUPT, "Wrong timings format %s" % timings)
		if not 'alpha_channel' in timings:
			close(CORRUPT, "Wrong timings format %s" % timings)

		if not type(timings['red_channel']) is int:
			close(CORRUPT, "Wrong timings format %s" % timings)
		if not type(timings['green_channel']) is int:
			close(CORRUPT, "Wrong timings format %s" % timings)
		if not type(timings['blue_channel']) is int:
			close(CORRUPT, "Wrong timings format %s" % timings)
		if not type(timings['alpha_channel']) is int:
			close(CORRUPT, "Wrong timings format %s" % timings)
	
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
		r = requests.post(url, headers={'signature': signatureBase64, 'User-Agent' : UserAgents.get()})
		if r.status_code == 404:
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
		r = requests.get(url, headers={'User-Agent' : UserAgents.get()})
	except Exception as e:
		 close(DOWN, "HTTP Error", "HTTP error: %s" % e)

	if r.status_code == 404:
		close(DOWN, "Service is down, status code=%u" % r.status_code)
	if r.status_code != 200:
		close(CORRUPT, "Status code=%u" % r.status_code)		

	if r.text.find(flag_id) == -1:
		close(CORRUPT, "There is no kernel with id '%s'" % flag_id)

	key = M2Crypto.EVP.load_key('privkey.pem')
	key.reset_context(md='sha256')
	key.sign_init()
	cipher = ''.join(random.choice(string.ascii_uppercase + string.digits) for i in range(32))
	key.sign_update(cipher.encode('utf-8'))
	signature = key.sign_final()
	signatureBase64 = base64.b64encode(signature)

	url = 'http://%s:8081/get-kernel?kernel-id=%s' % (addr, flag_id)
	try:
		r = requests.get(url, headers={'signature': signatureBase64, 'cipher' : cipher, 'User-Agent' : UserAgents.get()})
		if r.status_code == 404:
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
