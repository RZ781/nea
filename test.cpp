#include <iostream>
#include "cpu.h"
#include "arm.h"

int main() {
	int count = 0;
	for (int i = 0; i<256; i++) {
		for (int j =0;j<256; j++) {
			uint16_t x = (i << 8) | j;
			ArmInstruction i(x);
			if (i.opcode == OPCODE_UNKNOWN) {
				std::cout << "0 ";
			} else {
				std::cout << "1 ";
				count++;
			}
		}
		std::cout << '\n';
	}
	std::cout << count << '\n';
}
