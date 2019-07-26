#!/usr/bin/env bash


sudo docker run --rm --net=host -v `pwd`/checker:/home/checker \
                                -ti binderchecker ./checker.py $@
