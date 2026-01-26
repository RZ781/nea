#include <map>
#include <iostream>
#include <string>
#include <vector>
#include "cpu.h"
#include <libriscv/machine.hpp>
#include <libriscv/util/load_binary_file.hpp>

RiscVCPU::RiscVCPU(void):
	guest_data(load_binary_file("./a.out")),
	machine(guest_data),
	running(true)
{
	machine.setup_linux({"./a.out"}, {"LC_TYPE=C", "LC_ALL=C", "USER=user"});
	machine.setup_linux_syscalls();
}

void RiscVCPU::step(void) {
	if (!running)
		return;
	try {
		machine.cpu.step_one();
	} catch (std::exception& e) {
		std::cout << "error: " << e.what() << '\n';
		running = false;
	}
	std::cout.flush();
}

std::map<std::string, int> RiscVCPU::get_registers(void) {
	std::map<std::string, int> output;
	output["pc"] = machine.cpu.pc();
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
