#!/bin/bash

ip=localhost
if [ $# -eq 1 ]
then
  ip=$1
fi

rnd=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
FLAG="FLAG=$rnd"

echo ./check.sh check $ip
./check.sh check $ip
echo Status: $?
echo ./check.sh put $ip qweqweqweqweq $FLAG 1
flagid=`./check.sh put $ip qweqweqweqweq $FLAG 1`
echo Status: $?
echo Flag id: $flagid
echo ./check.sh get $ip $flagid $FLAG
./check.sh get $ip $flagid $FLAG
echo Status: $?
