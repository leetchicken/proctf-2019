import sys
import os
import re

name = sys.argv[1]

os.system("arm-none-eabi-nm -C %s.elf > /tmp/symbols" % name)
regExp = re.compile( r'(.+)\s\w\s(.*)\s')

f = open("/tmp/symbols", "r")
for line in f:
    match = regExp.match(line)
    if match.group(2) == "GameMain(API*, unsigned char*)":
        addrStr = match.group(1)

addr = int("0x" + addrStr, 16).to_bytes(4, byteorder="little", signed=False);

finalBinFile = open("code.bin", "wb")
finalBinFile.write(addr)
bin = open("%s.bin" % name, "rb").read()
finalBinFile.write(bin)