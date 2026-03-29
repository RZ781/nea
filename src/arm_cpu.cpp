#include <headers.h>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <format>
#include "cpu.h"
#include "arm.h"

ArmCPU::ArmCPU(std::string filename):
	running(true)
{
	// generate flat binary
	std::string command = std::format("arm-none-eabi-objcopy -O binary {} binary", filename);
	Glib::spawn_command_line_sync(command, nullptr, nullptr, nullptr);
	// find entry point
	std::string output;
	Glib::spawn_command_line_sync("arm-none-eabi-readelf -h " + filename, &output, nullptr, nullptr);
	std::stringstream ss;
	ss << std::hex << output.substr(output.find("Entry point address:") + 21);;
	uint32_t entry_point;
	ss >> entry_point;
	// load file
	std::ifstream binary("binary");
	binary.read((char*) memory+0x8000, sizeof(memory));
	registers[15] = entry_point | 1;
	registers[13] = (sizeof(memory) - 1) & ~3;
}

uint8_t ArmCPU::get_byte(uint32_t address) {
	return memory[address & (sizeof(memory) - 1)];
}

void ArmCPU::set_byte(uint32_t address, uint8_t value) {
	memory[address & (sizeof(memory) - 1)] = value;
}

uint32_t ArmCPU::get(uint32_t address) {
	uint32_t value = 0;
	for (int i = 0; i < 4; i++) {
		value |= get_byte(address + i) << (i * 8);
	}
	return value;
}

void ArmCPU::set(uint32_t address, uint32_t value) {
	for (int i = 0; i < 4; i++) {
		set_byte(address + i, (value >> (i * 8)) & 0xFF);
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
	{0xBC00, 0xFE00, OPCODE_POP, {.immediate={0, 0xFF, 0, 8, 0x8000}}},
	{0x1000, 0xF800, OPCODE_ASR_IMMEDIATE, {.immediate={6, 0x1F}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0xB100, 0xFD00, OPCODE_CBZ, {.immediate={3, 0x1F, 1, 9, 0x40}, .destination={0, 0x7}}},
	{0x4300, 0xFFC0, OPCODE_ORR_REGISTER, {.destination={0, 0x7}, .source={0, 0x7}, .source2={3, 0x7}, .set_flags=true}},
	{0x0800, 0xF800, OPCODE_LSR_IMMEDIATE, {.immediate={6, 0x1F}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x4300, 0xFF00, OPCODE_CMP_REGISTER, {.source={0, 0x7, 0, 7, 0x8}, .source2={3, 0xF}}},
	{0x1A00, 0xFE00, OPCODE_SUB_REGISTER, {.destination={0, 0x7}, .source={3, 0x7}, .source2={6, 0x7}}},
	{0x4280, 0xFFC0, OPCODE_CMP_REGISTER, {.source={0, 0x7}, .source2={3, 0x7}}},
	{0x4140, 0xFFC0, OPCODE_ADC_REGISTER, {.destination={0, 0x7}, .source={0, 0x7}, .source2={3, 0x7}, .set_flags=true}},
	{0xB2C0, 0xFFC0, OPCODE_UXTB, {.destination={0, 0x7}, .source{3, 0x7}}},
	{0x7000, 0xF800, OPCODE_STRB_IMMEDIATE, {.immediate={6, 0x1F, 2}, .source={0, 0x7}, .source2={3, 0x7}}},
	{0x4000, 0xFFC0, OPCODE_AND_REGISTER, {.destination={0, 0x7}, .source={0, 0x7}, .source2={3, 0x7}, .set_flags=true}},
	{0xC000, 0xF800, OPCODE_STM, {.immediate={0, 0xFF}, .source={8, 0x7}}},
	{0x5000, 0xFE00, OPCODE_STR_REGISTER, {.destination={0, 0x7}, .source={3, 0x7}, .source2={6, 0x7}}},
	{0xC800, 0xF800, OPCODE_LDM, {.immediate={0, 0xFF}, .source={8, 0x7}}},
	{0x4200, 0xFFC0, OPCODE_TST_REGISTER, {.source={0, 0x7}, .source2={3, 0x7}}},
	{0x9000, 0xF800, OPCODE_STR_IMMEDIATE, {.immediate={0, 0xFF, 2}, .source={8, 0x7}, .source2={0, 0, 0, 15, 13}}},
	{0x4780, 0xFF80, OPCODE_BLX_REGISTER, {.source={3, 0xF}}},
	{0x9800, 0xF800, OPCODE_LDR_IMMEDIATE, {.immediate={0, 0xFF, 2}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x4240, 0xFFC0, OPCODE_RSB_IMMEDIATE, {.immediate={0, 0}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0x5E00, 0xFE00, OPCODE_LDRSH_REGISTER, {.destination={0, 0x7}, .source={3, 0x7}, .source2={6, 0x7}}},
	{0x1E00, 0xFE00, OPCODE_SUB_IMMEDIATE, {.immediate={6, 0x7}, .destination={0, 0x7}, .source={3, 0x7}, .set_flags=true}},
	{0x3800, 0xF800, OPCODE_SUB_IMMEDIATE, {.immediate={0, 0xFF}, .destination={8, 0x7}, .source={8, 0x7}, .set_flags=true}},
	{0x4080, 0xFFC0, OPCODE_LSL_REGISTER, {.destination={0, 0x7}, .source={0, 0x7}, .source2={3, 0x7}}},
	{0x8000, 0xF800, OPCODE_STRH_IMMEDIATE, {.immediate={6, 0x1F, 1}, .source={0, 0x7}, .source2={3, 0x7}}},
	{0x8800, 0xF800, OPCODE_LDRH_IMMEDIATE, {.immediate={6, 0x1F, 1}, .destination={0, 0x7}, .source={3, 0x7}}},
	{0xBA00, 0xFFC0, OPCODE_REV, {.destination={0, 0x7}, .source={3, 0x7}}},
	{0x5800, 0xFE00, OPCODE_LDR_REGISTER, {.destination={0, 0x7}, .source={3, 0x7}, .source2={6, 0x7}}},
	{0x5C00, 0xFE00, OPCODE_LDRB_REGISTER, {.destination={0, 0x7}, .source={3, 0x7}, .source2={6, 0x7}}},
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
	uint16_t top_5 = word1 >> 11;
	if (top_5 == 0b11101 || top_5 == 0b11110 || top_5 == 0b11111) {
		length = 4;
	} else {
		length = 2;
	}
}

bool add_carry(uint32_t x, uint32_t y, bool carry) {
	uint64_t result = (uint64_t) x + (uint64_t) y + carry;
	return (result >> 32) > 0;
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

uint32_t ArmInstruction::add_set_flags(uint32_t x, uint32_t y, bool carry, ArmCPU& cpu) {
	uint32_t result = x + y + carry;
	cpu.condition_n = result >> 31;
	cpu.condition_z = result == 0;
	cpu.condition_c = add_carry(x, y, carry);
	cpu.condition_v = add_overflow(x, y, carry);
	return result;
}

void ArmInstruction::syscall(ArmCPU& cpu) {
	uint32_t arg1 = cpu.registers[0];
	uint32_t arg2 = cpu.registers[1];
	uint32_t arg3 = cpu.registers[2];
	switch (cpu.registers[7]) {
		case 1: // exit
			cpu.running = false;
			break;
		case 4: // write
			cpu.registers[0] = write(arg1, cpu.memory + arg2, arg3);
			break;
		case 6: // close
			cpu.registers[0] = 0;
			break;
		default:
			std::cout << "warning: unknown system call " << cpu.registers[7] << '\n';
			std::cout << arg1 << ' ' << arg2 << ' ' << arg3 << '\n';
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
			syscall(cpu);
			break;
		case OPCODE_ADD_REGISTER:
			cpu.registers[destination] = add_set_flags(cpu.registers[source], cpu.registers[source2], 0, cpu);
			break;
		case OPCODE_LSL_IMMEDIATE:
			cpu.registers[destination] = cpu.registers[source] << immediate;
			break;
		case OPCODE_BL_IMMEDIATE:
			cpu.registers[14] = pc | 1;
			next_pc = pc + ((int32_t) immediate | 1);
			break;
		case OPCODE_ADD_IMMEDIATE:
			cpu.registers[destination] = add_set_flags(cpu.registers[source], immediate, 0, cpu);
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
			add_set_flags(cpu.registers[source], ~immediate, 1, cpu);
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
		case OPCODE_ASR_IMMEDIATE:
			if (immediate == 0) {
				cpu.registers[destination] = ((int32_t) cpu.registers[source]) >= 0 ? 0 : -1;
			} else {
				cpu.registers[destination] = cpu.registers[source] >> immediate;
			}
			break;
		case OPCODE_MOVT:
			cpu.registers[destination] &= 0xFFFF;
			cpu.registers[destination] |= immediate << 16;
			break;
		case OPCODE_CBZ:
			if (cpu.registers[source] == 0) {
				next_pc = pc + ((int32_t) immediate | 1);
			}
			break;
		case OPCODE_ORR_REGISTER:
			cpu.registers[destination] = cpu.registers[source] | cpu.registers[source2];
			if (set_flags) {
				cpu.condition_n = cpu.registers[destination] >> 31;
				cpu.condition_z = cpu.registers[destination] == 0;
			}
			break;
		case OPCODE_LSR_IMMEDIATE:
			if (immediate == 0) {
				cpu.registers[destination] = 0;
			} else {
				cpu.registers[destination] = cpu.registers[source] >> immediate;
			}
			break;
		case OPCODE_CMP_REGISTER:
			add_set_flags(cpu.registers[source], ~cpu.registers[source2], 1, cpu);
			break;
		case OPCODE_SUB_REGISTER:
			cpu.registers[destination] = add_set_flags(cpu.registers[source], ~cpu.registers[source2], 1, cpu);
			break;
		case OPCODE_ADC_REGISTER:
			cpu.registers[destination] = add_set_flags(cpu.registers[source], cpu.registers[source2], cpu.condition_c, cpu);
			break;
		case OPCODE_UXTB:
			cpu.registers[destination] = cpu.registers[source];
			break;
		case OPCODE_STRB_IMMEDIATE:
			cpu.set_byte(cpu.registers[source2] + immediate, cpu.registers[source]);
			break;
		case OPCODE_AND_REGISTER:
			cpu.registers[destination] = cpu.registers[source] & cpu.registers[source2];
			if (set_flags) {
				cpu.condition_n = cpu.registers[destination] >> 31;
				cpu.condition_z = cpu.registers[destination] == 0;
			}
			break;
		case OPCODE_STM:
		{
			uint32_t address = cpu.registers[source];
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i)) {
					cpu.set(address, cpu.registers[i]);
					address += 4;
				}
			}
			cpu.registers[source] = address;
			break;
		}
		case OPCODE_STR_REGISTER:
			cpu.set(cpu.registers[source] + cpu.registers[source2], cpu.registers[destination]);
			break;
		case OPCODE_LDM:
		{
			uint32_t address = cpu.registers[source];
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i)) {
					cpu.registers[i] = cpu.get(address);
					address += 4;
				}
			}
			if ((immediate & (1 << source)) == 0) {
				cpu.registers[source] = address;
			}
			break;
		}
		case OPCODE_TST_REGISTER:
		{
			uint32_t result = cpu.registers[source] & cpu.registers[source2];
			cpu.condition_n = result >> 31;
			cpu.condition_z = result == 0;
			break;
		}
		case OPCODE_BLX_REGISTER:
			cpu.registers[14] = pc | 1;
			next_pc = cpu.registers[source];
			break;
		case OPCODE_RSB_IMMEDIATE:
			cpu.registers[destination] = add_set_flags(~cpu.registers[source], immediate, 1, cpu);
			break;
		case OPCODE_LDRSH_REGISTER:
			cpu.registers[destination] = cpu.get(cpu.registers[source] + cpu.registers[source2]) & 0xFFFF;
			break;
		case OPCODE_SUB_IMMEDIATE:
			cpu.registers[destination] = add_set_flags(cpu.registers[source], ~immediate, 1, cpu);
			break;
		case OPCODE_LSL_REGISTER:
			if (cpu.registers[source2] >= 32) {
				cpu.registers[destination] = 0;
			} else {
				cpu.registers[destination] = cpu.registers[source] << cpu.registers[source2];
			}
			break;
		case OPCODE_STRH_IMMEDIATE:
		{
			uint32_t address = cpu.registers[source2] + immediate;
			cpu.set_byte(address, cpu.registers[source] & 0xFF);
			cpu.set_byte(address + 1, (cpu.registers[source] >> 8) & 0xFF);
			break;
		}
		case OPCODE_LDRH_IMMEDIATE:
			cpu.registers[destination] = cpu.get(cpu.registers[source] + immediate) & 0xFFFF;
			break;
		case OPCODE_REV:
		{
			uint32_t reversed = 0;
			for (int i = 0; i < 4; i++) {
				uint8_t byte = cpu.registers[source] >> (i * 8);
				reversed |= byte << (24 - i * 8);
			}
			cpu.registers[destination] = reversed;
			break;
		}
		case OPCODE_LDR_REGISTER:
			cpu.registers[destination] = cpu.get(cpu.registers[source] + cpu.registers[source2]);
			break;
		case OPCODE_LDRB_REGISTER:
			cpu.registers[destination] = cpu.get(cpu.registers[source] + cpu.registers[source2]) & 0xFF;
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
	"ASR_IMMEDIATE",
	"MOVT",
	"CBZ",
	"ORR_REGISTER",
	"LSR_IMMEDIATE",
	"CMP_REGISTER",
	"SUB_REGISTER",
	"ADC_REGISTER",
	"UXTB",
	"STRB_IMMEDIATE",
	"AND_IMMEDIATE",
	"STM",
	"STR_REGISTER",
	"LDM",
	"TST_REGISTER",
	"BLX_REGISTER",
	"RSB_IMMEDIATE",
	"LDRSH_REGISTER",
	"SUB_IMMEDIATE",
	"LSL_REGISTER",
	"STRH_IMMEDIATE",
	"LDRH_IMMEDIATE",
	"REV",
	"LDR_REGISTER",
	"LDRB_REGISTER",
};

std::string ArmInstruction::disassemble(void) {
	switch (opcode) {
		case OPCODE_UNKNOWN:
			return std::format(".word 0x{:X}", (uint32_t) (word1 | (word2 << 16)));
		case OPCODE_MOV_IMMEDIATE:
			return std::format("mov{} r{}, #{}", set_flags ? "s" : "", destination, immediate);
		case OPCODE_LDR_LITERAL:
			return std::format("ldr r{}, [pc, #{}]", destination, immediate);
		case OPCODE_SVC:
			return std::format("svc {}", immediate);
		case OPCODE_ADD_REGISTER:
			return std::format("add r{}, r{}, r{}", destination, source, source2);
		case OPCODE_LSL_IMMEDIATE:
			return std::format("lsl r{}, r{}, #{}", destination, source, immediate);
		case OPCODE_BL_IMMEDIATE:
			return std::format("bl <pc+{}>", immediate);
		case OPCODE_ADD_IMMEDIATE:
			return std::format("add r{}, r{}, #{}", destination, source, immediate);
		case OPCODE_BX:
			return std::format("bx r{}", source);
		case OPCODE_PUSH:
			return std::format("push registers {}", immediate);
		case OPCODE_SUB_SP_IMMEDIATE:
			return std::format("sub sp, #{}", immediate);
		case OPCODE_ADD_SP_IMMEDIATE:
			return std::format("sub r{}, sp, #{}", destination, immediate);
		case OPCODE_STR_IMMEDIATE:
			return std::format("str r{}, [r{}, #{}]", source, source2, immediate);
		case OPCODE_LDR_IMMEDIATE:
			return std::format("ldr r{}, [r{}, #{}]", destination, source, immediate);
		case OPCODE_B:
			return std::format("b <pc+{}>", immediate);
		case OPCODE_LDRB_IMMEDIATE:
			return std::format("ldrb r{}, [r{}, #{}]", destination, source, immediate);
		case OPCODE_CMP_IMMEDIATE:
			return std::format("cmp r{}, #{}", source, immediate);
		case OPCODE_B_CONDITIONAL:
			return std::format("b.{} <pc+{}>", (int) condition, immediate);
	}
	return std::format("{} r{} r{} r{} 0x{:X}", opcode_names[(int) opcode], destination, source, source2, immediate);
}

int ArmInstruction::get_length(void) {
	return length;
}
