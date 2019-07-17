#!/bin/bash

# Usage:
# 1. install GCC ARM
# 2. sudo ./install_dependencies <path to GCC ARM/bin>
#    sudo ./install_dependencies /opt/gcc-arm-none-eabi-8-2018-q4-major/bin

if [ -z "$1" ]; then
    echo "You need to specify path to GCC ARM"
    exit 1
fi

apt-get install python2.7 python-pip
dpkg --add-architecture i386
apt-get update
apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386
pip install mbed-cli
mbed config -G GCC_ARM_PATH $1
pip install -r hw/mbed-os/requirements.txt
apt install libpugixml-dev libmicrohttpd-dev

