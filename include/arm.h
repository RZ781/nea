#ifndef ARM_H
#define ARM_H

#include "cpu.h"

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
	OPCODE_STRB_IMMEDIATE
};

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

struct OperandLocation {
	int shift_right;
	uint16_t mask;
	int shift_left;
	int high_bit;
	uint32_t high_bit_mask;
	uint32_t get_operand(uint16_t instruction);
};

struct Operands {
	OperandLocation immediate, destination, source, source2, condition;
	bool set_flags;
};

struct Arm16BitEncoding {
	uint16_t pattern;
	uint16_t pattern_mask;
	ArmOpcode opcode;
	Operands operands;
};

class ArmInstruction {
	private:
	uint16_t word1, word2;
	ArmOpcode opcode;
	bool set_flags;
	int destination, source, source2;
	uint32_t immediate;
	ArmCondition condition;
	int length;
	public:
	ArmInstruction(uint32_t data);
	std::string disassemble(void);
	void run(ArmCPU& cpu);
	int get_length(void);
};

#endif
