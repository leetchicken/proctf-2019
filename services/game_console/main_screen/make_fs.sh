#!/bin/bash
mkdir /tmp/fs
mount -o loop fs.bin /tmp/fs
cp code.bin /tmp/fs
cp mac /tmp/fs
../build_bmp/build_bmp assets/background.png /tmp/fs/background.bmp
../build_bmp/build_bmp assets/empty_icon.png /tmp/fs/empty_icon.bmp
../build_bmp/build_bmp assets/loading.png /tmp/fs/loading.bmp
../build_bmp/build_bmp assets/network_off.png /tmp/fs/network_off.bmp
../build_bmp/build_bmp assets/network_on.png /tmp/fs/network_on.bmp
../build_bmp/build_bmp assets/refresh.png /tmp/fs/refresh.bmp
umount /tmp/fs
rm -r /tmp/fs
