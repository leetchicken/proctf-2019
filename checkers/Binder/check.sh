#!/usr/bin/env bash


docker run --rm --net=host -v `pwd`/checker:/home/checker \
                           -i binderchecker ./checker.py $@
