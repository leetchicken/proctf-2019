#!/bin/bash

ip=localhost
if [ $# -eq 1 ]
then
  ip=$1
fi

echo ./check.sh check $ip
./check.sh check $ip
echo Status: $?
echo ./check.sh put $ip qweqweqweqweq FLLLAAAAGGGG==
flagid=`./check.sh put $ip qweqweqweqweq FLLLAAAAGGGG==`
echo Status: $?
echo Flag id: $flagid
echo ./check.sh get $ip $flagid FLLLAAAAGGGG==
./check.sh get $ip $flagid FLLLAAAAGGGG==
echo Status: $?
