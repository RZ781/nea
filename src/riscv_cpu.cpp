#include <map>
#include <iostream>
#include <string>
#include <vector>
#include "cpu.h"
#include <libriscv/machine.hpp>
#include <libriscv/util/load_binary_file.hpp>
#include <libriscv/rv32i_instr.hpp>

RiscVCPU::RiscVCPU(std::string filename):
	guest_data(load_binary_file(filename)),
	machine(guest_data),
	running(true)
{
	// initialise command line arguments and environment variables of program
	machine.setup_linux({filename}, {"LC_TYPE=C", "LC_ALL=C", "USER=user"});
	machine.setup_linux_syscalls();
}

// get value at memory address
uint32_t RiscVCPU::get(uint32_t address) {
	uint32_t value = machine.memory.read<uint32_t>(address);
	return value;
}

// run one instruction
void RiscVCPU::step(void) {
	if (!running)
		return;
	try {
		machine.cpu.step_one();
	} catch (std::exception& e) {
		// stop if there is an error
		std::cout << "error: " << e.what() << '\n';
		running = false;
	}
	// flush output to make sure everything gets printed immediately
	std::cout.flush();
}

// return map of registers
std::map<std::string, int> RiscVCPU::get_registers(void) {
	std::map<std::string, int> output;
	// set program counter
	output["pc"] = machine.cpu.pc();
	// set general purpose registers x0-x31
	for (int i = 0; i < 32; i++) {
		std::string name = "x";
		name += std::to_string(i);
		output[name] = machine.cpu.reg(i);
	}
	return output;
}

// disassemble a range of memory
std::vector<std::string> RiscVCPU::disassemble(int start, int end) {
	std::vector<std::string> output;
	for (int i = start; i <= end; i += 4) {
		riscv::instruction_format instruction(get(i));
		output.push_back(machine.cpu.to_string(instruction));
	}
	return output;
}
