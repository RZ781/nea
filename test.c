#include <stdint.h>
#include <linux/unistd.h>

uint32_t syscall(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t n);

const char msg[] = "Hello, world!\n";

__attribute__((naked))
void _start() {
	syscall(1, (uint32_t) msg, sizeof(msg), __NR_write);
	syscall(0, 0, 0, __NR_exit);
}
