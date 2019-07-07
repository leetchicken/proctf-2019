#!/bin/bash
mkdir /tmp/fs
mount -o loop fs.bin /tmp/fs
cp code.bin /tmp/fs
umount /tmp/fs