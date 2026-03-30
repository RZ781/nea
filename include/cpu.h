#ifndef CPU_H
#define CPU_H

#include <map>
#include <string>
#include <vector>
#include <libriscv/machine.hpp>

// abstract class all cpus inherit from
class CPU {
	public:
	virtual void step(void) = 0;
	virtual std::map<std::string, int> get_registers(void) = 0;
	virtual std::vector<std::string> disassemble(int start, int end) = 0;
	virtual uint32_t get(uint32_t address) = 0;
};

// default cpu when no real cpu is loaded
class ExampleCPU: public CPU {
	private:
	int program_counter;
	public:
	ExampleCPU(void);
	void step(void) override;
	std::map<std::string, int> get_registers(void) override;
	std::vector<std::string> disassemble(int start, int end) override;
	uint32_t get(uint32_t address);
};

// riscv cpu based on libriscv
class RiscVCPU: public CPU {
	private:
	std::vector<unsigned char> guest_data;
	riscv::Machine<riscv::RISCV32> machine;
	bool running;
	public:
	RiscVCPU(std::string filename);
	void step(void) override;
	std::map<std::string, int> get_registers(void) override;
	std::vector<std::string> disassemble(int start, int end) override;
	uint32_t get(uint32_t address);
};

// arm cpu
class ArmCPU: public CPU {
	private:
	// 16 general purpose registers, including program counter
	uint32_t registers[16];
	// 64 mb (2^26 bytes) of ram
	uint8_t memory[1<<26];
	// whether or not the cpu is running
	bool running;
	// condition flags
	bool condition_n, condition_z, condition_c, condition_v;
	void set(uint32_t address, uint32_t value);
	uint8_t get_byte(uint32_t address);
	void set_byte(uint32_t address, uint8_t value);
	public:
	ArmCPU(std::string filename);
	void step(void) override;
	std::map<std::string, int> get_registers(void) override;
	std::vector<std::string> disassemble(int start, int end) override;
	uint32_t get(uint32_t address);
	// allow arm instructions to change private attributes of arm cpu
	friend class ArmInstruction;
};

#endif
