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


def gen_signature(msg):
	key = M2Crypto.EVP.load_key('privkey.pem')
	key.reset_context(md='sha256')
	key.sign_init()
	key.sign_update(msg.encode('utf-8'))
	signature = key.sign_final()
	signatureBase64 = base64.b64encode(signature)
	return signatureBase64


def close(code, public="", private=""):
	if public:
		print(public)
	if private:
		print(private, file=stderr)
	print('Exit with code %d' % code, file=stderr)
	exit(code)


def check(*args):
	addr = args[0]

	url = 'http://%s/list' % (addr)
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
	kernel_id = lines[random.randint(0, len(lines) - 2)]

	# generate images
	imgStreams = {}
	images = {}
	for i in range(random.randint(1, 16)):
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
		images.update({name : img})

	# ask service to process generated images
	sign = (random.randint(0, 256) % 8) == 0
	url = 'http://%s/process?kernel-id=%s' % (addr, kernel_id)
	try:
		headers={'User-Agent' : UserAgents.get()}
		if sign:
			signatureBase64 = gen_signature(kernel_id)
			headers.update({'signature': signatureBase64})
		
		r = requests.post(url, headers=headers, files=imgStreams)
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

	if sign:
		signatureBase64 = gen_signature(kernel_id)
		url = 'http://%s/get-kernel?kernel-id=%s' % (addr, kernel_id)
		try:
			r = requests.get(url, headers={'signature': signatureBase64, 'User-Agent' : UserAgents.get()})
			if r.status_code == 404:
				close(DOWN, "Service is down, status code=%u" % r.status_code)
			if r.status_code != 200:
				close(CORRUPT, "Service is corrupted, status code=%u" % r.status_code)
		except Exception as e:
			close(DOWN, "HTTP Error", "HTTP error: %s" % e)

		kernel = r.text
		if len(kernel) != 32:
			close(CORRUPT, "Wrong kernel returned", "Len: %u" % len(kernel))

		# verify that convolution works well
		for name, sourceImage in images.items():
			signatureBase64 = gen_signature(name)
			url = 'http://%s/get-image?name=%s' % (addr, name)
			try:
				r = requests.get(url, headers={'User-Agent' : UserAgents.get(), 'signature' : signatureBase64})
			except Exception as e:
				close(DOWN, "HTTP Error", "HTTP error: %s" % e)

			if r.status_code == 404:
				close(DOWN, "Service is down, status code=%u" % r.status_code)
			if r.status_code != 200:
				close(CORRUPT, "Status code=%u" % r.status_code)

			try:
				processedImage = Image.open(io.BytesIO(r.content))
			except Exception as e:
				close(DOWN, "Non PNG image returned", "%s" % e)

			expectedWidth = ((sourceImage.width + 23) // 24) * 8
			expectedHeight = (sourceImage.height + 2) // 3
			if expectedWidth != processedImage.width or expectedHeight != processedImage.height:
				close(CORRUPT, "Wrong image returned", "Expected %ux%u but %ux%u" % (expectedWidth, expectedHeight, processedImage.width, processedImage.height))

			srcPixels = sourceImage.load()
			processedPixels = processedImage.load()
			# perform convolution and compare with processed by service image
			for i in range(0, 4):
				for y in range(0, processedImage.height):
					for x in range(0, processedImage.width):
						x0 = x * 3
						y0 = y * 3
						v = [0, 0, 0, 0, 0, 0, 0, 0]
						if x0 + 0 < sourceImage.width and y0 + 0 < sourceImage.height:
							v[0] = srcPixels[x0 + 0, y0 + 0][i]
						if x0 + 1 < sourceImage.width and y0 + 0 < sourceImage.height:
							v[1] = srcPixels[x0 + 1, y0 + 0][i]
						if x0 + 2 < sourceImage.width and y0 + 0 < sourceImage.height:
							v[2] = srcPixels[x0 + 2, y0 + 0][i]
						if x0 + 0 < sourceImage.width and y0 + 1 < sourceImage.height:
							v[3] = srcPixels[x0 + 0, y0 + 1][i]
						if x0 + 2 < sourceImage.width and y0 + 1 < sourceImage.height:
							v[4] = srcPixels[x0 + 2, y0 + 1][i]
						if x0 + 0 < sourceImage.width and y0 + 2 < sourceImage.height:
							v[5] = srcPixels[x0 + 0, y0 + 2][i]
						if x0 + 1 < sourceImage.width and y0 + 2 < sourceImage.height:
							v[6] = srcPixels[x0 + 1, y0 + 2][i]
						if x0 + 2 < sourceImage.width and y0 + 2 < sourceImage.height:
							v[7] = srcPixels[x0 + 2, y0 + 2][i]
						
						counter = 0.0
						sum = 0
						for j in range(0, 8):
							if v[j] >= ord(kernel[i * 8 + j]):
								sum += v[j]
								counter += 1.0

						r = 0
						if counter > 0:
							r = int(round(sum / counter))

						testVal = processedPixels[x, y][i]
						if testVal != r:
							close(CORRUPT, "Wrong image returned", "Expected %u but %u at %ux%u[%u]" % (r, testVal, x, y, i))
	
	close(OK)


def put(*args):
	addr = args[0]
	flag_id = args[1]
	flag = args[2]

	signatureBase64 = gen_signature(flag_id)
	url = 'http://%s/add-kernel?kernel-id=%s&kernel=%s' % (addr, flag_id, flag)
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

	url = 'http://%s/list' % (addr)
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

	signatureBase64 = gen_signature(flag_id)
	url = 'http://%s/get-kernel?kernel-id=%s' % (addr, flag_id)
	try:
		r = requests.get(url, headers={'signature': signatureBase64, 'User-Agent' : UserAgents.get()})
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
