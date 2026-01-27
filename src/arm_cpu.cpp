#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "cpu.h"
#include "arm.h"

ArmCPU::ArmCPU(void):
	running(true)
{
	std::ifstream binary("binary");
	binary.read((char*) memory+0x10074, sizeof(memory));
	registers[15] = 0x10074;
}

uint32_t ArmCPU::get(uint32_t address) {
	uint32_t value = 0;
	for (int i = 0; i < 4; i++) {
		value |= memory[address + i] << (i * 8);
	}
	return value;
}

void ArmCPU::step(void) {
	if (!running)
		return;
	uint32_t pc = registers[15];
	ArmInstruction instruction(get(pc));
	instruction.run(*this);
}

std::map<std::string, int> ArmCPU::get_registers(void) {
	std::map<std::string, int> output;
	output["pc"] = registers[15];
	for (int i = 0; i < 15; i++) {
		std::string name = "r";
		name += std::to_string(i);
		output[name] = registers[i];
	}
	return output;
}

std::vector<std::string> ArmCPU::disassemble(int start, int end) {
	std::vector<std::string> output;
	for (int i = start; i <= end; i += 4) {
		output.push_back(std::to_string(i));
	}
	return output;
}

ArmInstruction::ArmInstruction(uint32_t value) {
	word1 = value & 0xFFFF;
	word2 = value >> 16;
	uint16_t op = word1 >> 10;
	if (op >> 1 == 0b00100) {
		opcode = OPCODE_MOV_IMMEDIATE;
		immediate = word1 & 0xFF;
		destination = (word1 >> 8) & 0x7;
		set_flags = true;
	} else {
		opcode = OPCODE_UNKNOWN;
	}
}

void ArmInstruction::run(ArmCPU& cpu) {
	switch (opcode) {
		case OPCODE_UNKNOWN:
			std::cout << "error: unknown instruction " << std::hex << word1 << ' ' << word2 << std::dec << "\n";
			break;
		case OPCODE_MOV_IMMEDIATE:
			cpu.registers[destination] = immediate;
			if (set_flags) {
				cpu.condition_z = immediate == 0;
				cpu.condition_n = immediate >> 31;
			}
			break;
		default:
			std::cout << "error: unimplemented opcode " << opcode << '\n';
	}
}
