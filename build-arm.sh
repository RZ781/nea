#!/bin/sh

arm-unknown-linux-musleabi-gcc test.c -o a.out -static -ffreestanding -nostdlib -mthumb -lc -lgcc -lc -lgcc
arm-unknown-linux-musleabi-objcopy -O binary a.out binary
arm-unknown-linux-musleabi-objdump a.out -f
arm-unknown-linux-musleabi-objdump a.out -h
