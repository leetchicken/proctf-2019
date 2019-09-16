#!/bin/sh
/opt/wasi-sdk/bin/clang -fno-inline -Ofast polyfill.c -o polyfill.wasm
