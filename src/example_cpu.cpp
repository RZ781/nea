#include <map>
#include <string>
#include <vector>
#include "cpu.h"

ExampleCPU::ExampleCPU(void) {}

// run one instruction
void ExampleCPU::step(void) {
	program_counter += 4;
}

// get data at address
uint32_t ExampleCPU::get(uint32_t address) {
	return address;
}

// return map of registers
std::map<std::string, int> ExampleCPU::get_registers(void) {
	std::map<std::string, int> output;
	output["pc"] = program_counter;
	return output;
}

// disassemble a range of memory
std::vector<std::string> ExampleCPU::disassemble(int start, int end) {
	std::vector<std::string> output = {"No program loaded"};
	return output;
}
