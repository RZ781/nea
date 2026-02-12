#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

uint32_t syscall(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t n);

void log(int number) {
	syscall(number, number, number, 6767);
}

void _exit(int status) {
	syscall(status, 0, 0, 1);
}

int _read(int file, char* ptr, int len) {
	return syscall(file, (uint32_t) ptr, len, 3);
}

int _write(int file, char* ptr, int len) {
	return syscall(file, (uint32_t) ptr, len, 4);
}

int _isatty(int fd) {
	log(0);
	return 0;
}

int _lseek(int fd, int offset, int whence) {
	return syscall(fd, offset, whence, 19);
}

int _close(int fd) {
	return syscall(fd, 0, 0, 6);
}

void* _sbrk(int nbytes) {
	log(1);
	return NULL;
}

int _fstat(int fd, struct stat* buf) {
	return syscall(fd, (uint32_t) buf, 0, 108);
}

int main() {
	fwrite("hello world", 11, 1, stdout);
	_exit(0);
}
