#!/bin/sh

rm a.out binary
arm-none-eabi-gcc test.c test.S -o a.out -static -mcpu=cortex-m0 -mthumb
arm-none-eabi-objcopy -O binary a.out binary
arm-none-eabi-objdump a.out -f
arm-none-eabi-objdump a.out -h
