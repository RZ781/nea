#include <map>
#include <iostream>
#include <string>
#include <vector>
#include "cpu.h"
#include <libriscv/machine.hpp>
#include <libriscv/util/load_binary_file.hpp>
#include <libriscv/rv32i_instr.hpp>

RiscVCPU::RiscVCPU(void):
	guest_data(load_binary_file("./a.out")),
	machine(guest_data),
	running(true)
{
	machine.setup_linux({"./a.out"}, {"LC_TYPE=C", "LC_ALL=C", "USER=user"});
	machine.setup_linux_syscalls();
}

uint32_t RiscVCPU::get(uint32_t address) {
	uint32_t value = machine.memory.read<uint32_t>(address);
	return value;
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
		riscv::instruction_format instruction(get(i));
		//char buffer[512];
		//instruction.printer(buffer, sizeof(buffer), machine.cpu, instruction);
		output.push_back(machine.cpu.to_string(instruction));
	}
	return output;
}
