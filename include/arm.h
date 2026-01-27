#ifndef ARM_H
#define ARM_H

#include "cpu.h"

enum ArmOpcode {
	OPCODE_UNKNOWN,
	OPCODE_MOV_IMMEDIATE,

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

struct ArmInstruction {
	uint16_t word1, word2;
	ArmOpcode opcode;
	bool set_flags;
	int destination;
	uint32_t immediate;

	ArmInstruction(uint32_t data);
	int length();
	std::string disassemble(void);
	void run(ArmCPU& cpu);
};

#endif
