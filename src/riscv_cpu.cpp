#include <map>
#include <iostream>
#include <string>
#include <vector>
#include "cpu.h"
#include <libriscv/machine.hpp>
#include <libriscv/util/load_binary_file.hpp>

RiscVCPU::RiscVCPU(void):
	guest_data(load_binary_file("./binary")),
	machine(guest_data)
{ }

void RiscVCPU::step(void) {
	machine.simulate(1);
}

std::map<std::string, int> RiscVCPU::get_registers(void) {
	std::map<std::string, int> output;
	for (int i = 0; i < 32; i++) {
		std::string name = "x";
		name += std::to_string(i);
		output[name] = machine.cpu.reg(i);
	}
	return output;
}

std::vector<std::string> RiscVCPU::disassemble(int start, int end) {
	std::vector<std::string> output;
	for (int i = start; i <= end; i += 4) {
		output.push_back(std::to_string(i));
	}
	return output;
}
