#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <format>
#include "cpu.h"
#include "arm.h"

ArmCPU::ArmCPU(void):
	running(true)
{
	std::ifstream binary("binary");
	binary.read((char*) memory+0x10074, sizeof(memory));
	registers[15] = 0x100e3;
	registers[13] = (sizeof(memory) - 1) & ~3;
}

uint32_t ArmCPU::get(uint32_t address) {
	uint32_t value = 0;
	for (int i = 0; i < 4; i++) {
		value |= memory[address + i] << (i * 8);
	}
	return value;
}

void ArmCPU::set(uint32_t address, uint32_t value) {
	for (int i = 0; i < 4; i++) {
		memory[address + i] = (value >> (i * 8)) & 0xFF;
	}
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
	output["APSR.N"] = condition_n;
	output["APSR.Z"] = condition_z;
	output["APSR.C"] = condition_c;
	output["APSR.V"] = condition_v;
	for (int i = 0; i < 15; i++) {
		std::string name = "r";
		name += std::to_string(i);
		output[name] = registers[i];
	}
	return output;
}

std::vector<std::string> ArmCPU::disassemble(int start, int end) {
	std::vector<std::string> output;
	int address = start & ~1;
	while (address < end) {
		ArmInstruction instruction(get(address));
		std::string string = std::format(" {} 0x{:X}: {}", address == (registers[15] & ~1) ? '>' : ' ', address, instruction.disassemble());
		output.push_back(string);
		address += instruction.get_length();
	}
	return output;
}

uint32_t OperandLocation::get_operand(uint16_t instruction) {
	uint32_t value = instruction >> shift_right;
	value &= mask;
	value <<= shift_left;
	if (high_bit) {
		bool bit = instruction & (1 << high_bit);
		if (bit)
			value |= high_bit_mask;
	}
	return value;
}

Arm16BitEncoding encoding_table[] = {
	{0x2000, 0xF800, OPCODE_MOV_IMMEDIATE, {.immediate={0, 0xFF}, .destination={8, 0x7}, .set_flags=true}},
	{0x4800, 0xF800, OPCODE_LDR_LITERAL, {.immediate={0, 0xFF, 2}, .destination={8, 0x7}}},
	{0xDF00, 0xFF00, OPCODE_SVC, {.immediate={0, 0xFF}}},
	{0x4400, 0xFF00, OPCODE_ADD_REGISTER, {.destination={0, 0x7, 0, 7, 0x8}, .source={0, 0x7, 0, 7, 0x8}, .source2={3, 0xF}, .set_flags=false}},
	{0x0000, 0xF800, OPCODE_LSL_IMMEDIATE, {.immediate={6, 0x1F}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x1C00, 0xFE00, OPCODE_ADD_IMMEDIATE, {.immediate={6, 0x7}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x4700, 0xFF80, OPCODE_BX, {.source={3, 0xF}}},
	{0xB400, 0xFE00, OPCODE_PUSH, {.immediate={0, 0xFF, 0, 8, 0x4000}}},
	{0xB080, 0xFF80, OPCODE_SUB_SP_IMMEDIATE, {.immediate={0, 0x7F, 2}}},
	{0xA800, 0xF800, OPCODE_ADD_SP_IMMEDIATE, {.immediate={0, 0xFF}, .destination={8, 0x7}}},
	{0x6000, 0xF800, OPCODE_STR_IMMEDIATE, {.immediate={6, 0x1F, 2}, .source={0, 0x7}, .source2={3, 0x7}}},
	{0x6800, 0xF800, OPCODE_LDR_IMMEDIATE, {.immediate={6, 0x1F, 2}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0xE000, 0xF800, OPCODE_B, {.immediate={0, 0x7FF, 1, 10, 0xFFFFF800}}},
	{0x1800, 0xFE00, OPCODE_ADD_REGISTER, {.destination={0, 0x7}, .source={6, 0x7}, .source2={3, 0x7}}},
	{0x7800, 0xF800, OPCODE_LDRB_IMMEDIATE, {.immediate={6, 0x1F, 2}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x2800, 0xF800, OPCODE_CMP_IMMEDIATE, {.immediate={0, 0xFF}, .source={8, 0x7}}},
	{0xD000, 0xF000, OPCODE_B_CONDITIONAL, {.immediate={0, 0xFF, 1, 7, 0xFFFFFF00}, .condition={8, 0xF}}},
	{0x3000, 0xF800, OPCODE_ADD_IMMEDIATE, {.immediate={0, 0xFF}, .destination={8, 0x7}, .source={8, 0x7}}},
	{0x4600, 0xFF00, OPCODE_MOV_REGISTER, {.destination={0, 0x7, 0, 7, 0x8}, .source={3, 0xF}}},
	{0xB000, 0xFF80, OPCODE_ADD_SP_IMMEDIATE, {.immediate={0, 0x7F, 2}, .destination={0, 0, 0, 15, 13}}},
	{0xBC00, 0xFE00, OPCODE_POP, {.immediate={0, 0xFF, 0, 8, 0x8000}}}
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
			condition = (ArmCondition) encoding.operands.condition.get_operand(word1);
			set_flags = encoding.operands.set_flags;
			length = 2;
			return;
		}
	}
	opcode = OPCODE_UNKNOWN;
	length = 4;
}


bool add_carry(uint32_t x, uint32_t y, bool carry) {
	uint64_t result = x + y + carry;
	return result > 0xFFFFFFFF;
}

bool add_overflow(int32_t x, int32_t y, bool carry) {
	int32_t result = x + y + carry;
	bool x_sign = x >= 0;
	bool y_sign = y >= 0;
	bool result_sign = result >= 0;
	return x_sign == y_sign && x_sign != result_sign;
}

bool evaluate_condition(ArmCondition condition, bool n, bool z, bool c, bool v) {
	switch (condition) {
		case CONDITION_EQ: return z == 1;
		case CONDITION_NE: return z == 0;
		case CONDITION_CS: return c == 1;
		case CONDITION_CC: return c == 0;
		case CONDITION_MI: return n == 1;
		case CONDITION_PL: return n == 0;
		case CONDITION_VS: return v == 1;
		case CONDITION_VC: return v == 0;
		case CONDITION_HI: return c == 1 && z == 0;
		case CONDITION_LS: return c == 0 || z == 1;
		case CONDITION_GE: return n == v;
		case CONDITION_LT: return n != v;
		case CONDITION_GT: return z == 0 && n == v;
		case CONDITION_LE: return z == 1 || n != v;
		default: return true;
	}
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
			next_pc = cpu.registers[source];
			break;
		case OPCODE_PUSH:
		{
			int n_registers = 0;
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i))
					n_registers++;
			}
			uint32_t address = cpu.registers[13] - n_registers * 4;
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i)) {
					cpu.set(address, cpu.registers[i]);
					address += 4;
				}
			}
			cpu.registers[13] -= n_registers * 4;
			break;
		}
		case OPCODE_SUB_SP_IMMEDIATE:
			cpu.registers[13] -= immediate;
			break;
		case OPCODE_ADD_SP_IMMEDIATE:
			cpu.registers[destination] = cpu.registers[13] + immediate;
			break;
		case OPCODE_STR_IMMEDIATE:
			cpu.set(cpu.registers[source2] + immediate, cpu.registers[source]);
			break;
		case OPCODE_LDR_IMMEDIATE:
			cpu.registers[destination] = cpu.get(cpu.registers[source] + immediate);
			break;
		case OPCODE_B:
			next_pc = pc + ((int32_t) immediate | 1);
			break;
		case OPCODE_LDRB_IMMEDIATE:
			cpu.registers[destination] = cpu.get(cpu.registers[source] + immediate) & 0xFF;
			break;
		case OPCODE_CMP_IMMEDIATE:
			cpu.condition_n = (int32_t) (cpu.registers[source] - immediate) < 0;
			cpu.condition_z = cpu.registers[source] == immediate;
			cpu.condition_c = add_carry(cpu.registers[source], ~immediate, 1);
			cpu.condition_v = add_overflow(cpu.registers[source], ~immediate, 1);
			break;
		case OPCODE_B_CONDITIONAL:
			if (evaluate_condition(condition, cpu.condition_n, cpu.condition_z, cpu.condition_c, cpu.condition_v)) {
				next_pc = pc + ((int32_t) immediate | 1);
			}
			break;
		case OPCODE_MOV_REGISTER:
			cpu.registers[destination] = cpu.registers[source];
			break;
		case OPCODE_POP:
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i)) {
					cpu.registers[i] = cpu.get(cpu.registers[13]);
					cpu.registers[13] += 4;
				}
			}
			if (immediate & (1 << 15)) {
				next_pc = cpu.get(cpu.registers[13]);
				cpu.registers[13] += 4;
			}
			break;
		default:
			std::cout << "warning: unimplemented opcode " << opcode << '\n';
	}
	cpu.registers[15] = next_pc;
}

std::string opcode_names[] = {
	"UNKNOWN",
	"MOV_IMMEDIATE",
	"LDR_LITERAL",
	"SVC",
	"ADD_REGISTER",
	"LSL_IMMEDIATE",
	"BL_IMMEDIATE",
	"ADD_IMMEDIATE",
	"BX",
	"PUSH",
	"SUB_SP_IMMEDIATE",
	"ADD_SP_IMMEDIATE",
	"STR_IMMEDIATE",
	"LDR_IMMEDIATE",
	"B",
	"LDRB_IMMEDIATE",
	"CMP_IMMEDIATE",
	"B_CONDITIONAL",
	"MOV_REGISTER",
	"POP",
};

std::string ArmInstruction::disassemble(void) {
	if (opcode == OPCODE_UNKNOWN) {
		return std::format(".word 0x{:X}", (uint32_t) (word1 | (word2 << 16)));
	}
	return std::format("{} r{} r{} r{} 0x{:X}", opcode_names[(int) opcode], destination, source, source2, immediate);
}

int ArmInstruction::get_length(void) {
	return length;
}
