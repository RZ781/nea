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
	syscall(code, 0, 0, 1);
}

void print(const char* string) {
	syscall(1, (uint32_t) string, length(string), 4);
}

void print_int(int x) {
	char string[10];
	int i = 0;
	while (x) {
		string[i] = x % 10 + '0';
		x /= 10;
		i++;
	}
	string[i] = 0;
	print(string);
}

__attribute__((naked))
void _start() {
	print_int(67);
	exit(0);
}
