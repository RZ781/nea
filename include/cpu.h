#ifndef CPU_H
#define CPU_H

#include <map>
#include <string>
#include <vector>
#include <libriscv/machine.hpp>

class CPU {
	public:
	virtual void step(void) = 0;
	virtual std::map<std::string, int> get_registers(void) = 0;
	virtual std::vector<std::string> disassemble(int start, int end) = 0;
};

class ExampleCPU: public CPU {
	private:
	int program_counter;
	public:
	ExampleCPU(void);
	void step(void) override;
	std::map<std::string, int> get_registers(void) override;
	std::vector<std::string> disassemble(int start, int end) override;
};

class RiscVCPU: public CPU {
	private:
	std::vector<unsigned char> guest_data;
	riscv::Machine<riscv::RISCV32> machine;
	bool running;
	public:
	RiscVCPU(void);
	void step(void) override;
	std::map<std::string, int> get_registers(void) override;
	std::vector<std::string> disassemble(int start, int end) override;
};

class ArmCPU: public CPU {
	private:
	uint32_t registers[16];
	uint8_t memory[1<<26];
	bool running;
	bool condition_n, condition_z, condition_c, condition_v;
	uint32_t get(uint32_t address);
	void set(uint32_t address, uint32_t value);
	public:
	ArmCPU(void);
	void step(void) override;
	std::map<std::string, int> get_registers(void) override;
	std::vector<std::string> disassemble(int start, int end) override;
	friend class ArmInstruction;
};

#endif
