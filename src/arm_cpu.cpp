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
	// load file into memory
	std::ifstream binary("binary");
	binary.read((char*) memory+0x8000, sizeof(memory));
	// initialise program counter to entry point
	registers[15] = entry_point | 1;
	// initialise stack pointer to just before end of memory, since stack grows downwards
	registers[13] = (sizeof(memory) - 1) & ~3;
}

// read one byte from memory with bounds checking
uint8_t ArmCPU::get_byte(uint32_t address) {
	return memory[address & (sizeof(memory) - 1)];
}

// write one byte to memory with bounds checking
void ArmCPU::set_byte(uint32_t address, uint8_t value) {
	memory[address & (sizeof(memory) - 1)] = value;
}

// read 32-bit value from memory
uint32_t ArmCPU::get(uint32_t address) {
	uint32_t value = 0;
	for (int i = 0; i < 4; i++) {
		value |= get_byte(address + i) << (i * 8);
	}
	return value;
}

// write 32-bit value to memory
void ArmCPU::set(uint32_t address, uint32_t value) {
	for (int i = 0; i < 4; i++) {
		set_byte(address + i, (value >> (i * 8)) & 0xFF);
	}
}

// run one instruction
void ArmCPU::step(void) {
	if (!running)
		return;
	// fetch instruction from memory
	uint32_t pc = registers[15];
	if ((pc & 1) == 0) {
		std::cout << "error: arm mode not supported\n";
		return;
	}
	// instruction is decoded by constructor
	ArmInstruction instruction(get(pc & ~1));
	// run instruction
	instruction.run(*this);
}

// get map of registers
std::map<std::string, int> ArmCPU::get_registers(void) {
	std::map<std::string, int> output;
	// add special purpose registers (pc, condition flags)
	output["pc"] = registers[15];
	output["APSR.N"] = condition_n;
	output["APSR.Z"] = condition_z;
	output["APSR.C"] = condition_c;
	output["APSR.V"] = condition_v;
	// add general purpose registers r0-r14
	for (int i = 0; i < 15; i++) {
		std::string name = "r";
		name += std::to_string(i);
		output[name] = registers[i];
	}
	return output;
}

// disassemble a memory range
std::vector<std::string> ArmCPU::disassemble(int start, int end) {
	std::vector<std::string> output;
	// align address to 2 bytes
	int address = start & ~1;
	while (address < end) {
		// decode instruction with constructor
		ArmInstruction instruction(get(address));
		// disassemble instruction and format in its address
		std::string string = std::format(" {} 0x{:X}: {}", address == (registers[15] & ~1) ? '>' : ' ', address, instruction.disassemble());
		output.push_back(string);
		address += instruction.get_length();
	}
	return output;
}

// extract operand from an instruction
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

// list of instruction encodings
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

// instruction constructor which decodes instructions
ArmInstruction::ArmInstruction(uint32_t value) {
	// arm uses little endian, so least significant bits come first
	word1 = value & 0xFFFF;
	word2 = value >> 16;
	// BL (branch and link) has a 32-bit encoding, which cannot be decoded using the 16-bit encoding table
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
		// check if this encoding's pattern matches with the instruction
		if ((word1 & encoding.pattern_mask) == encoding.pattern) {
			// set opcode
			opcode = encoding.opcode;
			// extract each operand
			immediate = encoding.operands.immediate.get_operand(word1);
			destination = encoding.operands.destination.get_operand(word1);
			source = encoding.operands.source.get_operand(word1);
			source2 = encoding.operands.source2.get_operand(word1);
			condition = (ArmCondition) encoding.operands.condition.get_operand(word1);
			set_flags = encoding.operands.set_flags;
			// length is always 2 bytes since this a 16 bit instruction
			length = 2;
			return;
		}
	}
	// no encoding matched
	opcode = OPCODE_UNKNOWN;
	// these 3 bit patterns are for 4 byte instructions, otherwise it is 2 byte
	uint16_t top_5 = word1 >> 11;
	if (top_5 == 0b11101 || top_5 == 0b11110 || top_5 == 0b11111) {
		length = 4;
	} else {
		length = 2;
	}
}

// check if carry flag should be set
bool add_carry(uint32_t x, uint32_t y, bool carry) {
	// check for unsigned overflow
	uint64_t result = (uint64_t) x + (uint64_t) y + carry;
	return (result >> 32) > 0;
}

// check if overflow flag should be set
bool add_overflow(int32_t x, int32_t y, bool carry) {
	int32_t result = x + y + carry;
	bool x_sign = x >= 0;
	bool y_sign = y >= 0;
	bool result_sign = result >= 0;
	// check if the inputs have the same sign but the result is different
	return x_sign == y_sign && x_sign != result_sign;
}

// evaluate if a conditional instruction should be run given the condition codes
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

// set all 4 flags appropriately for an addition
uint32_t ArmInstruction::add_set_flags(uint32_t x, uint32_t y, bool carry, ArmCPU& cpu) {
	uint32_t result = x + y + carry;
	// top bit of an unsigned number is 1 if the signed number would be negative
	cpu.condition_n = result >> 31;
	cpu.condition_z = result == 0;
	cpu.condition_c = add_carry(x, y, carry);
	cpu.condition_v = add_overflow(x, y, carry);
	return result;
}

// run a system call
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

// run an instruction
void ArmInstruction::run(ArmCPU& cpu) {
	// find the next value for the program counter
	uint32_t next_pc = cpu.registers[15] + length;
	// get the value that the program counter is read as, which is 4 bytes after the actual address
	uint32_t pc = (cpu.registers[15] & ~1) + 4;
	cpu.registers[15] = pc;
	switch (opcode) {
		case OPCODE_UNKNOWN:
			std::cout << "warning: unknown instruction " << std::hex << word1 << ' ' << word2 << std::dec << "\n";
			break;
		case OPCODE_MOV_IMMEDIATE:
			// set destination to immediate and update flags
			cpu.registers[destination] = immediate;
			if (set_flags) {
				cpu.condition_z = immediate == 0;
				cpu.condition_n = immediate >> 31;
			}
			break;
		case OPCODE_LDR_LITERAL:
			// set destination to value at pc-relative address
			cpu.registers[destination] = cpu.get((pc & ~3) + immediate);
			break;
		case OPCODE_SVC:
			// run a system call
			syscall(cpu);
			break;
		case OPCODE_ADD_REGISTER:
			// set destination to sum of sources, and set flags
			cpu.registers[destination] = add_set_flags(cpu.registers[source], cpu.registers[source2], 0, cpu);
			break;
		case OPCODE_LSL_IMMEDIATE:
			// set destination to source shifted left by immediate
			cpu.registers[destination] = cpu.registers[source] << immediate;
			break;
		case OPCODE_BL_IMMEDIATE:
			// jump to pc-relative address and update link register to previous address so it can be returned to
			cpu.registers[14] = pc | 1;
			next_pc = pc + ((int32_t) immediate | 1);
			break;
		case OPCODE_ADD_IMMEDIATE:
			// set destination to sum of source and immediate, and set flags
			cpu.registers[destination] = add_set_flags(cpu.registers[source], immediate, 0, cpu);
			break;
		case OPCODE_BX:
			// jump to register
			next_pc = cpu.registers[source];
			break;
		case OPCODE_PUSH:
		{
			// count how many registers are being pushed
			int n_registers = 0;
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i))
					n_registers++;
			}
			// push each register to memory
			uint32_t address = cpu.registers[13] - n_registers * 4;
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i)) {
					cpu.set(address, cpu.registers[i]);
					address += 4;
				}
			}
			// update stack pointer
			cpu.registers[13] -= n_registers * 4;
			break;
		}
		case OPCODE_SUB_SP_IMMEDIATE:
			// subtract stack pointer by immediate
			cpu.registers[13] -= immediate;
			break;
		case OPCODE_ADD_SP_IMMEDIATE:
			// set destination to stack pointer + immediate
			cpu.registers[destination] = cpu.registers[13] + immediate;
			break;
		case OPCODE_STR_IMMEDIATE:
			// store source register into address immediate + source2
			cpu.set(cpu.registers[source2] + immediate, cpu.registers[source]);
			break;
		case OPCODE_LDR_IMMEDIATE:
			// load data at address immediate + source
			cpu.registers[destination] = cpu.get(cpu.registers[source] + immediate);
			break;
		case OPCODE_B:
			// jump to pc relative address
			next_pc = pc + ((int32_t) immediate | 1);
			break;
		case OPCODE_LDRB_IMMEDIATE:
			// load one byte at address source + immediate
			cpu.registers[destination] = cpu.get(cpu.registers[source] + immediate) & 0xFF;
			break;
		case OPCODE_CMP_IMMEDIATE:
			// set flags as if immediate was subtracted from source
			add_set_flags(cpu.registers[source], ~immediate, 1, cpu);
			break;
		case OPCODE_B_CONDITIONAL:
			// jump to pc relative address if condition is true
			if (evaluate_condition(condition, cpu.condition_n, cpu.condition_z, cpu.condition_c, cpu.condition_v)) {
				next_pc = pc + ((int32_t) immediate | 1);
			}
			break;
		case OPCODE_MOV_REGISTER:
			// set destination to source
			cpu.registers[destination] = cpu.registers[source];
			break;
		case OPCODE_POP:
			// pop each register r0-r14
			for (int i = 0; i < 15; i++) {
				if (immediate & (1 << i)) {
					cpu.registers[i] = cpu.get(cpu.registers[13]);
					cpu.registers[13] += 4;
				}
			}
			// if r15 (program counter) is popped, do a jump
			if (immediate & (1 << 15)) {
				next_pc = cpu.get(cpu.registers[13]);
				cpu.registers[13] += 4;
			}
			break;
		case OPCODE_ASR_IMMEDIATE:
			// set destination to source shifted right by immediate (signed)
			if (immediate == 0) {
				cpu.registers[destination] = ((int32_t) cpu.registers[source]) >= 0 ? 0 : -1;
			} else {
				cpu.registers[destination] = cpu.registers[source] >> immediate;
			}
			break;
		case OPCODE_MOVT:
			// set the top 16 bits of destination to immediate
			cpu.registers[destination] &= 0xFFFF;
			cpu.registers[destination] |= immediate << 16;
			break;
		case OPCODE_CBZ:
			// jump if source is zero
			if (cpu.registers[source] == 0) {
				next_pc = pc + ((int32_t) immediate | 1);
			}
			break;
		case OPCODE_ORR_REGISTER:
			// set destination to sources bitwise ored together, set flags
			cpu.registers[destination] = cpu.registers[source] | cpu.registers[source2];
			if (set_flags) {
				cpu.condition_n = cpu.registers[destination] >> 31;
				cpu.condition_z = cpu.registers[destination] == 0;
			}
			break;
		case OPCODE_LSR_IMMEDIATE:
			// set destination to source shifted righted by immediate, or just 0 if immediate is 0
			if (immediate == 0) {
				cpu.registers[destination] = 0;
			} else {
				cpu.registers[destination] = cpu.registers[source] >> immediate;
			}
			break;
		case OPCODE_CMP_REGISTER:
			// set flags as if source2 was subtracted from source1
			add_set_flags(cpu.registers[source], ~cpu.registers[source2], 1, cpu);
			break;
		case OPCODE_SUB_REGISTER:
			// set destination to source - source2 and set flags
			cpu.registers[destination] = add_set_flags(cpu.registers[source], ~cpu.registers[source2], 1, cpu);
			break;
		case OPCODE_ADC_REGISTER:
			// set destination to source + source2 + carry flag
			cpu.registers[destination] = add_set_flags(cpu.registers[source], cpu.registers[source2], cpu.condition_c, cpu);
			break;
		case OPCODE_UXTB:
			// set destination to lower byte of source
			cpu.registers[destination] = cpu.registers[source] & 0xFF;
			break;
		case OPCODE_STRB_IMMEDIATE:
			// store the lower byte of source
			cpu.set_byte(cpu.registers[source2] + immediate, cpu.registers[source]);
			break;
		case OPCODE_AND_REGISTER:
			// set destination to source & source2 and set flags
			cpu.registers[destination] = cpu.registers[source] & cpu.registers[source2];
			if (set_flags) {
				cpu.condition_n = cpu.registers[destination] >> 31;
				cpu.condition_z = cpu.registers[destination] == 0;
			}
			break;
		case OPCODE_STM:
		{
			// store registers specified by immediate's bits to memory at address source
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
			// store register in memory
			cpu.set(cpu.registers[source] + cpu.registers[source2], cpu.registers[destination]);
			break;
		case OPCODE_LDM:
		{
			// load registers specified by immediate's bits from memory at address source
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
			// set flags as if sources were anded together
			uint32_t result = cpu.registers[source] & cpu.registers[source2];
			cpu.condition_n = result >> 31;
			cpu.condition_z = result == 0;
			break;
		}
		case OPCODE_BLX_REGISTER:
			// jump to register and set previous pc to link register so it can be returned to
			cpu.registers[14] = pc | 1;
			next_pc = cpu.registers[source];
			break;
		case OPCODE_RSB_IMMEDIATE:
			// set destination to immediate - source and set flags
			cpu.registers[destination] = add_set_flags(~cpu.registers[source], immediate, 1, cpu);
			break;
		case OPCODE_LDRSH_REGISTER:
			// load lower two bytes at address source + source2
			cpu.registers[destination] = cpu.get(cpu.registers[source] + cpu.registers[source2]) & 0xFFFF;
			break;
		case OPCODE_SUB_IMMEDIATE:
			// set destination to source - immediate and set flags
			cpu.registers[destination] = add_set_flags(cpu.registers[source], ~immediate, 1, cpu);
			break;
		case OPCODE_LSL_REGISTER:
			// set destination to source shifted left by source2
			if (cpu.registers[source2] >= 32) {
				cpu.registers[destination] = 0;
			} else {
				cpu.registers[destination] = cpu.registers[source] << cpu.registers[source2];
			}
			break;
		case OPCODE_STRH_IMMEDIATE:
		{
			// store the lower 2 bytes of source
			uint32_t address = cpu.registers[source2] + immediate;
			cpu.set_byte(address, cpu.registers[source] & 0xFF);
			cpu.set_byte(address + 1, (cpu.registers[source] >> 8) & 0xFF);
			break;
		}
		case OPCODE_LDRH_IMMEDIATE:
			// load 2 bytes at source + immediate
			cpu.registers[destination] = cpu.get(cpu.registers[source] + immediate) & 0xFFFF;
			break;
		case OPCODE_REV:
		{
			// set destination to source with bytes reversed
			uint32_t reversed = 0;
			for (int i = 0; i < 4; i++) {
				uint8_t byte = cpu.registers[source] >> (i * 8);
				reversed |= byte << (24 - i * 8);
			}
			cpu.registers[destination] = reversed;
			break;
		}
		case OPCODE_LDR_REGISTER:
			// load data at address source + source2
			cpu.registers[destination] = cpu.get(cpu.registers[source] + cpu.registers[source2]);
			break;
		case OPCODE_LDRB_REGISTER:
			// load one byte at address source + source2
			cpu.registers[destination] = cpu.get(cpu.registers[source] + cpu.registers[source2]) & 0xFF;
			break;
		default:
			std::cout << "warning: unimplemented opcode " << opcode << '\n';
	}
	// update program counter
	cpu.registers[15] = next_pc;
}

// list of opcode names for fallback disassembly
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

// disassemble an instruction
std::string ArmInstruction::disassemble(void) {
	// format correct assembly for each opcode
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
	// if this opcode isn't implemented, return opcode and operands even if not formatted properly
	return std::format("{} r{} r{} r{} 0x{:X}", opcode_names[(int) opcode], destination, source, source2, immediate);
}

// return length of instruction
int ArmInstruction::get_length(void) {
	return length;
}
