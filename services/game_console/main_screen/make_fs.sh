#!/bin/bash
mkdir /tmp/fs
mount -o loop fs.bin /tmp/fs
cp code.bin /tmp/fs
../build_bmp/build_bmp assets/background.png /tmp/fs/background.bmp
../build_bmp/build_bmp assets/empty_icon.png /tmp/fs/empty_icon.bmp
../build_bmp/build_bmp assets/loading.png /tmp/fs/loading.bmp
../build_bmp/build_bmp assets/network_off.png /tmp/fs/network_off.bmp
../build_bmp/build_bmp assets/network_on.png /tmp/fs/network_on.bmp
../build_bmp/build_bmp assets/refresh.png /tmp/fs/refresh.bmp
../build_bmp/build_bmp assets/account.png /tmp/fs/account.bmp
../build_bmp/build_bmp assets/button.png /tmp/fs/button.bmp
../build_bmp/build_bmp assets/capslock.png /tmp/fs/capslock.bmp
../build_bmp/build_bmp assets/backspace.png /tmp/fs/backspace.bmp
../build_bmp/build_bmp assets/cancel.png /tmp/fs/cancel.bmp
../build_bmp/build_bmp assets/accept.png /tmp/fs/accept.bmp
umount /tmp/fs
rm -r /tmp/fs
