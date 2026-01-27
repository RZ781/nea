#ifndef ARM_H
#define ARM_H

#include "cpu.h"

enum ArmOpcode {
	OPCODE_UNKNOWN,
	OPCODE_MOV_IMMEDIATE,
	OPCODE_LDR_LITERAL,
	OPCODE_SVC,
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
	uint16_t get_operand(uint16_t instruction);
};

struct Operands {
	OperandLocation immediate;
	OperandLocation destination;
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
	int destination;
	uint32_t immediate;
	int length;
	public:
	ArmInstruction(uint32_t data);
	std::string disassemble(void);
	void run(ArmCPU& cpu);
};

#endif
