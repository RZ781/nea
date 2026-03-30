#ifndef ARM_H
#define ARM_H

#include "cpu.h"

// list of arm opcodes
enum ArmOpcode {
	OPCODE_UNKNOWN,
	OPCODE_MOV_IMMEDIATE,
	OPCODE_LDR_LITERAL,
	OPCODE_SVC,
	OPCODE_ADD_REGISTER,
	OPCODE_LSL_IMMEDIATE,
	OPCODE_BL_IMMEDIATE,
	OPCODE_ADD_IMMEDIATE,
	OPCODE_BX,
	OPCODE_PUSH,
	OPCODE_SUB_SP_IMMEDIATE,
	OPCODE_ADD_SP_IMMEDIATE,
	OPCODE_STR_IMMEDIATE,
	OPCODE_LDR_IMMEDIATE,
	OPCODE_B,
	OPCODE_LDRB_IMMEDIATE,
	OPCODE_CMP_IMMEDIATE,
	OPCODE_B_CONDITIONAL,
	OPCODE_MOV_REGISTER,
	OPCODE_POP,
	OPCODE_ASR_IMMEDIATE,
	OPCODE_MOVT,
	OPCODE_CBZ,
	OPCODE_ORR_REGISTER,
	OPCODE_LSR_IMMEDIATE,
	OPCODE_CMP_REGISTER,
	OPCODE_SUB_REGISTER,
	OPCODE_ADC_REGISTER,
	OPCODE_UXTB,
	OPCODE_STRB_IMMEDIATE,
	OPCODE_AND_REGISTER,
	OPCODE_STM,
	OPCODE_STR_REGISTER,
	OPCODE_LDM,
	OPCODE_TST_REGISTER,
	OPCODE_BLX_REGISTER,
	OPCODE_RSB_IMMEDIATE,
	OPCODE_LDRSH_REGISTER,
	OPCODE_SUB_IMMEDIATE,
	OPCODE_LSL_REGISTER,
	OPCODE_STRH_IMMEDIATE,
	OPCODE_LDRH_IMMEDIATE,
	OPCODE_REV,
	OPCODE_LDR_REGISTER,
	OPCODE_LDRB_REGISTER,
};

// list of condition codes in conditional branches
enum ArmCondition {
	CONDITION_EQ,
	CONDITION_NE,
	CONDITION_CS,
	CONDITION_CC,
	CONDITION_MI,
	CONDITION_PL,
	CONDITION_VS,
	CONDITION_VC,
	CONDITION_HI,
	CONDITION_LS,
	CONDITION_GE,
	CONDITION_LT,
	CONDITION_GT,
	CONDITION_LE,
	CONDITION_NONE
};

// location of an operand within an instruction encoding
struct OperandLocation {
	// how many bits to shift right the instruction right before masking
	int shift_right;
	// bit mask to extract operand
	uint16_t mask;
	// how many bits to shift left after masking
	int shift_left;
	// bitwise or in the mask if the high bit is set
	int high_bit;
	uint32_t high_bit_mask;
	// extract the operand from an instruction
	uint32_t get_operand(uint16_t instruction);
};

// all of the operands that can be specified for an instruction encoding
struct Operands {
	OperandLocation immediate, destination, source, source2, condition;
	bool set_flags;
};

// instruction encoding
struct Arm16BitEncoding {
	// bits that are set to 1
	uint16_t pattern;
	// bit mask to check 
	uint16_t pattern_mask;
	// opcode of the encoding
	ArmOpcode opcode;
	// operand locations
	Operands operands;
};

// instruction
class ArmInstruction {
	private:
	// raw bytes of instruction
	uint16_t word1, word2;
	// whether or not to set cpu's condition flags
	bool set_flags;
	// source and destination registers
	int destination, source, source2;
	// immediate value
	uint32_t immediate;
	// condition code for conditional branches
	ArmCondition condition;
	// number of bytes the instruction is
	int length;
	public:
	ArmOpcode opcode;
	ArmInstruction(uint32_t data);
	std::string disassemble(void);
	void run(ArmCPU& cpu);
	uint32_t add_set_flags(uint32_t, uint32_t, bool, ArmCPU& cpu);
	void syscall(ArmCPU& cpu);
	int get_length(void);
};

#endif
