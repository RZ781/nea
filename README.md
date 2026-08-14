# NEA
This is my A-level computer science non-exam assessment. It is an Arm and RISC-V
emulator. I implemented the Arm instruction set myself, while RISC-V uses
[libriscv](https://github.com/libriscv/libriscv). If you are starting your NEA,
don't use this as inspiration. This caused me so much suffering.

## Building and running
Dependencies:
- g++
- make
- cmake
- gtkmm-4 (development package with headers)
- arm-none-eabi-gcc toolchain (optional)
- riscv32-unknown-linux-musl-gcc toolchain (optional)
The cross compilers are only needed to compile code within the emulator. I got
the GCC toolchains using crossdev on Gentoo, on other distros you may need to
manually build them from source if they are not in the repositories.
First, clone and build libriscv in this directory:
```
git clone https://github.com/libriscv/libriscv.git
cd libriscv
mkdir build
cd build
cmake ..
cmake --build .
```
Then, run `make` in this directory, building a binary called `vm`. Run this from
a terminal because it uses the terminal to show the emulated program's output.

## Features
Arm emulation:
- Most of the Thumb-1 instruction set
- write, exit, close system calls
- C standard library doesn't work
  - Most libc implementations require the full Arm instruction set or Thumb-2
  so they can't be used
  - I got newlib to work for my NEA writeup but I had to patch it to use normal
  Linux syscalls as it normally expects to run on embedded systems, and I don't
  have the code for this anymore
RISC-V emulation:
- All core instructions, any extensions supported by libriscv
- Many common system calls, as supported by libriscv
- C++ standard library doesn't work
  - This is only a problem with how the compiler is invoked so if you compile
  a program outside of the emulator and load the binary, it will work
