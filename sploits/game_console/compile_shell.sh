#!/bin/bash
arm-none-eabi-as shell.s -o shell.o
arm-none-eabi-objcopy -O binary shell.o shell.bin

