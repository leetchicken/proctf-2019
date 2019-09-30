#!/bin/sh

mkdir -p /home/polyfill/flags 2>/dev/null
chown polyfill:polyfill /home/polyfill/flags/
chmod 700 /home/polyfill/flags

exec xinetd -dontfork -f /home/polyfill/xinetd.conf
