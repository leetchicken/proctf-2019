#!/usr/bin/env python
import os
import re
import subprocess
import xml.etree.ElementTree as ET
import socket
import struct

# Usage:
# python build.py
#

regExp = re.compile(r'.+\s(.+)')
str = subprocess.check_output('mbed config -G GCC_ARM_PATH 2>/dev/null', shell=True)
gcc_arm = regExp.match(str).group(1)
print("GCC ARM Path: %s\n" % gcc_arm)
os.environ["PATH"] = os.environ["PATH"] + ":" + gcc_arm

os.system("mkdir BUILD; cd BUILD; rm -r *")

print("Build libpng")
if os.system("cd libpng; make") != 0:
    exit(1)
print("")

print("Build build_bmp")
if os.system("cd build_bmp; make") != 0:
    exit(1)
os.system("cp build_bmp/build_bmp BUILD/")
print("")

print("Build server")
os.system("mkdir BUILD/server")
if os.system("cd server; make clean; make -j8") != 0:
    exit(1)
os.system("cp server/server BUILD/server/")
os.system("cp server/docker_build.sh BUILD/server/")
os.system("cp server/Dockerfile BUILD/server/")
os.system("cp server/entrypoint.sh BUILD/server/")
os.system("mkdir BUILD/server/data")
os.system("cp server/data/*.xml BUILD/server/data")
os.system("cp server/data/*.html BUILD/server/data")
print("")

os.system("mkdir BUILD/SDK")

gamesXml = ET.ElementTree(file="server/data/games.xml").getroot()
for game in gamesXml:
	name = game.get("name")
	print("Build game: %s" % name)
	if os.system("cd server/data/%s; make" % name) != 0:
		exit(1)
	os.system("mkdir BUILD/server/data/%s" % name)
	os.system("cp server/data/%s/code.bin BUILD/server/data/%s/" % (name, name))
	os.system("cp server/data/%s/*.png BUILD/server/data/%s/" % (name, name))

	if game.get("sdk"):
		os.system("mkdir BUILD/SDK/%s" % name)
		os.system("cp server/data/%s/code.bin BUILD/SDK/%s/" % (name, name))
		os.system("cp server/data/%s/*.h BUILD/SDK/%s/" % (name, name))
		os.system("cp server/data/%s/*.cpp BUILD/SDK/%s/" % (name, name))
		os.system("cp server/data/%s/*.png BUILD/SDK/%s/" % (name, name))
		os.system("cp server/data/%s/makefile BUILD/SDK/%s/" % (name, name))
		os.system("cp server/data/%s/merge.py BUILD/SDK/%s/" % (name, name))
	print("")

os.system("cp hw/api.h BUILD/SDK/")
os.system("cd BUILD/SDK; zip SDK.zip `find .`")
os.system("mv BUILD/SDK/SDK.zip BUILD/server/data")
print("")

print("Build main_screen")
if os.system("cd main_screen; make") != 0:
    exit(1)
if os.system("cd main_screen; sudo ./make_fs.sh") != 0:
    exit(1)
os.system("cp main_screen/fs.bin BUILD/")
print("")

print("Build firmware")
if os.system("cd hw; ./compile_release.sh") != 0:
	exit(1)
os.system("cp hw/BUILD/DISCO_F746NG/GCC_ARM-RELEASE/hw.bin BUILD/")
os.system("cp hw/BUILD/DISCO_F746NG/GCC_ARM-RELEASE/hw.elf BUILD/")
print("")

