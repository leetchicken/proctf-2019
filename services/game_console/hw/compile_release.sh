#!/bin/bash
mbed compile --target DISCO_F746NG --toolchain GCC_ARM -v --profile release "$@"

