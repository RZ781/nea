#!/bin/sh

arm-none-eabi-gcc test.c test.S -o a.out -static -mthumb -nostdlib -ffreestanding -lgcc -mcpu=cortex-m0 -I/usr/include
arm-none-eabi-objcopy -O binary a.out binary
arm-none-eabi-objdump a.out -f
arm-none-eabi-objdump a.out -h
