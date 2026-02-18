#include <stdint.h>

uint32_t syscall(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t n);

uint32_t write(int fd, void* data, int count) {
	return syscall(fd, (uint32_t) data, count, 4);
}

void print_int(uint32_t x) {
	if (x == 0) {
		write(1, "0\n", 2);
		return;
	}
	char buf[11];
	int i = 10;
	buf[10] = 'A';
	buf[11] = 'B';
	while (x) {
		i--;
		buf[i] = x % 10 + '0';
		x /= 10;
	}
	write(1, buf + i, 11 - i);
	write(1, "\n", 1);
}

void print(char* x) {
	int i =0;
	while (x[i])
		i++;
	write(1, x, i);
}

int main() {
	int i = 0;
	while (1) {
		if (i % 3 == 0 && i % 5 == 0) {
			print("fizzbuzz\n");
		} else if (i % 3 == 0) {
			print("fizz\n");
		} else if (i % 5 == 0) {
			print("buzz\n");
		} else {
			print_int(i);
		}
		i++;
	}
}
