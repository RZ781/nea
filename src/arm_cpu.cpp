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
	for (int i =0; i<15;i++)
		registers[i]=i;
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

uint16_t OperandLocation::get_operand(uint16_t instruction) {
	return ((instruction >> shift_right) & mask) << shift_left;
}

Arm16BitEncoding encoding_table[] = {
	{0x2000, 0xF800, OPCODE_MOV_IMMEDIATE, {.immediate={0, 0xFF}, .destination={8, 0x7}, .set_flags=true}},
	{0x4800, 0xF800, OPCODE_LDR_LITERAL, {.immediate={0, 0xFF, 2}, .destination={8, 0x7}}},
	{0xDF00, 0xFF00, OPCODE_SVC, {.immediate={0, 0xFF}}}
};

ArmInstruction::ArmInstruction(uint32_t value) {
	word1 = value & 0xFFFF;
	word2 = value >> 16;
	for (Arm16BitEncoding encoding: encoding_table) {
		if ((word1 & encoding.pattern_mask) == encoding.pattern) {
			opcode = encoding.opcode;
			immediate = encoding.operands.immediate.get_operand(word1);
			destination = encoding.operands.destination.get_operand(word1);
			set_flags = encoding.operands.set_flags;
			length = 2;
			return;
		}
	}
	opcode = OPCODE_UNKNOWN;
}

void ArmInstruction::run(ArmCPU& cpu) {
	uint32_t pc = cpu.registers[15] + 4;
	cpu.registers[15] += length;
	switch (opcode) {
		case OPCODE_UNKNOWN:
			std::cout << "error: unknown instruction " << std::hex << word1 << ' ' << word2 << std::dec << "\n";
			cpu.registers[15] -= length;
			break;
		case OPCODE_MOV_IMMEDIATE:
			cpu.registers[destination] = immediate;
			if (set_flags) {
				cpu.condition_z = immediate == 0;
				cpu.condition_n = immediate >> 31;
			}
			break;
		case OPCODE_LDR_LITERAL:
			cpu.registers[destination] = cpu.get((pc & ~3) + immediate);
			break;
		case OPCODE_SVC:
			std::cout << "system call " << immediate << '\n';
			std::cout << "syscall number: " << cpu.registers[7] << '\n';
			std::cout << "args: ";
			for (int i = 0; i < 5; i++) {
				std::cout << cpu.registers[i] << ' ';
			}
			std::cout << '\n';
			if (cpu.registers[7] == 1) { // exit
				cpu.running = false;
			}
			break;
		default:
			std::cout << "error: unimplemented opcode " << opcode << '\n';
			cpu.registers[15] -= length;
	}
}
