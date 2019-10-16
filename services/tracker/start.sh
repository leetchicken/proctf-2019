#!/bin/sh

chown tracker: /home/tracker

su tracker -s /bin/sh -c './execute.py tracker.bin'
