#!/bin/bash

chown convolution: /home/convolution/data
chmod 755 /home/convolution /home/convolution/data

su convolution -s /bin/sh -c './convolution'