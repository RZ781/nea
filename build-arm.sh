#!/bin/sh

arm-unknown-linux-musleabi-gcc test.c test.S -o a.out -static -mthumb -nostdlib -ffreestanding -lgcc
arm-unknown-linux-musleabi-objcopy -O binary a.out binary
arm-unknown-linux-musleabi-objdump a.out -f
arm-unknown-linux-musleabi-objdump a.out -h
