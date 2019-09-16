#!/bin/sh

export HOME="/home/polyfill"

cd /home/polyfill
exec timeout -s9 60 /home/polyfill/.wasmtime/bin/wasmtime --cache --dir flags polyfill.wasm
