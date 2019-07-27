import sys
import os
import re

name = sys.argv[1]

os.system("arm-none-eabi-nm -C %s.elf > /tmp/symbols" % name)
regExp = re.compile( r'(.+)\s\w\s(.*)\s')

f = open("/tmp/symbols", "r")
for line in f:
    match = regExp.match(line)
    if match.group(2) == "GameInit(API*, unsigned char*)":
        gameInitAddrStr = match.group(1)

    if match.group(2) == "GameUpdate(API*, void*)":
        gameUpdateAddrStr = match.group(1)

finalBinFile = open("code.bin", "wb")

addr = int("0x" + gameInitAddrStr, 16).to_bytes(4, byteorder="little", signed=False);
finalBinFile.write(addr)

addr = int("0x" + gameUpdateAddrStr, 16).to_bytes(4, byteorder="little", signed=False);
finalBinFile.write(addr)

bin = open("%s.bin" % name, "rb").read()
finalBinFile.write(bin)