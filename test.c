#include <stdint.h>
#include <linux/unistd.h>

uint32_t syscall(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t n);

int length(const char* string) {
	int i = 0;
	while (string[i] != 0)
		i++;
	return i;
}

void exit(int code) {
	syscall(code, 0, 0, __NR_exit);
}

void print(const char* string) {
	syscall(1, (uint32_t) string, length(string), __NR_write);

}

__attribute__((naked))
void _start() {
	print("Hello, world!\n");
	exit(0);
}
