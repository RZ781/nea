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

#endif
