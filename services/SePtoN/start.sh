#!/bin/sh

export PYTHONUNBUFFERED=1

chown SePtoN: /home/SePtoN/data
chmod 755 /home/SePtoN
chmod 700 /home/SePtoN/data

su SePtoN -s /bin/sh -c './SePtoN 0.0.0.0 31337'

