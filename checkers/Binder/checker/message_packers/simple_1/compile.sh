#!/bin/bash

gcc -DMESLEN="$1" -DMES="$2" -DKEY="$4" template.cpp -o $3 -static
strip -S -g -M -x  $3
