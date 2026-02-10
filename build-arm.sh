#!/bin/sh

arm-none-eabi-gcc test.c test.S -o a.out -static -mthumb -nostdlib -ffreestanding -lgcc -I/usr/include -mcpu=cortex-m0
arm-none-eabi-objcopy -O binary a.out binary
arm-none-eabi-objdump a.out -f
arm-none-eabi-objdump a.out -h
