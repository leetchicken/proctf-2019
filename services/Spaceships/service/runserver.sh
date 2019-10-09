#!/bin/bash

screen -dmS ClearOld ./ClearLast20mins.py

mkdir -p spacemans
mkdir -p spaceships

while true; do
    socat -dd TCP4-LISTEN:3777,reuseaddr,fork,keepalive exec:./run.sh,end-close
    sleep 5
done
