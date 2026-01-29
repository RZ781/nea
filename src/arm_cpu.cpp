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
	registers[15] = 0x10075;
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
	if ((pc & 1) == 0) {
		std::cout << "error: arm mode not supported\n";
		return;
	}
	ArmInstruction instruction(get(pc & ~1));
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
	uint32_t value = instruction >> shift_right;
	value &= mask;
	value <<= shift_left;
	if (high_bit) {
		bool bit = instruction & (1 << high_bit);
		if (bit)
			value &= 0x8;
	}
	return value;
}

Arm16BitEncoding encoding_table[] = {
	{0x2000, 0xF800, OPCODE_MOV_IMMEDIATE, {.immediate={0, 0xFF}, .destination={8, 0x7}, .set_flags=true}},
	{0x4800, 0xF800, OPCODE_LDR_LITERAL, {.immediate={0, 0xFF, 2}, .destination={8, 0x7}}},
	{0xDF00, 0xFF00, OPCODE_SVC, {.immediate={0, 0xFF}}},
	{0x4400, 0xFF00, OPCODE_ADD_REGISTER, {.destination={0, 0x7, 0, 7}, .source={0, 0x7, 0, 7}, .source2={3, 0xF}, .set_flags=false}},
	{0x0000, 0xF800, OPCODE_LSL_IMMEDIATE, {.immediate={6, 0x1F}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x1C00, 0xFE00, OPCODE_ADD_IMMEDIATE, {.immediate={6, 0x7}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x4700, 0xFF80, OPCODE_BX, {.source={3, 0xF}}}
};

ArmInstruction::ArmInstruction(uint32_t value) {
	word1 = value & 0xFFFF;
	word2 = value >> 16;
	if ((word1 & 0xF800) == 0xF000 && (word2 & 0xD000) == 0xD000) { // BL immediate encoding T1
		opcode = OPCODE_BL_IMMEDIATE;
		immediate = (word2 & 0x7FF) << 1;
		immediate |= (word1 & 0x3FF) << 12;
		bool s = word1 & 0x0400;
		bool j1 = word2 & 0x2000;
		bool j2 = word2 & 0x0800;
		immediate |= !(s ^ j1) << 22;
		immediate |= !(s ^ j2) << 23;
		if (s) {
			immediate |= -1 << 24;
		}
		length = 4;
		return;
	}
	for (Arm16BitEncoding encoding: encoding_table) {
		if ((word1 & encoding.pattern_mask) == encoding.pattern) {
			opcode = encoding.opcode;
			immediate = encoding.operands.immediate.get_operand(word1);
			destination = encoding.operands.destination.get_operand(word1);
			source = encoding.operands.source.get_operand(word1);
			source2 = encoding.operands.source2.get_operand(word1);
			set_flags = encoding.operands.set_flags;
			length = 2;
			return;
		}
	}
	opcode = OPCODE_UNKNOWN;
}

void ArmInstruction::run(ArmCPU& cpu) {
	// todo: fix writing to pc
	uint32_t next_pc = cpu.registers[15] + length;
	uint32_t pc = (cpu.registers[15] & ~1) + 4;
	cpu.registers[15] = pc;
	switch (opcode) {
		case OPCODE_UNKNOWN:
			std::cout << "warning: unknown instruction " << std::hex << word1 << ' ' << word2 << std::dec << "\n";
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
		case OPCODE_ADD_REGISTER:
			cpu.registers[destination] = cpu.registers[source] + cpu.registers[source2];
			break;
		case OPCODE_LSL_IMMEDIATE:
			cpu.registers[destination] = cpu.registers[source] << immediate;
			break;
		case OPCODE_BL_IMMEDIATE:
			cpu.registers[14] = pc | 1;
			next_pc = pc + ((int32_t) immediate | 1);
			break;
		case OPCODE_ADD_IMMEDIATE:
			cpu.registers[destination] = cpu.registers[source] + immediate;
			break;
		case OPCODE_BX:
			next_pc = cpu.registers[source] | 1;
			break;
		default:
			std::cout << "warning: unimplemented opcode " << opcode << '\n';
	}
	cpu.registers[15] = next_pc;
}
