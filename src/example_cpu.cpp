#include <map>
#include <string>
#include <vector>
#include "cpu.h"

ExampleCPU::ExampleCPU(void) {}

void ExampleCPU::step(void) {
	program_counter += 4;
}

std::map<std::string, int> ExampleCPU::get_registers(void) {
	std::map<std::string, int> output;
	output["pc"] = program_counter;
	return output;
}

std::vector<std::string> ExampleCPU::disassemble(int start, int end) {
	std::vector<std::string> output;
	for (int i = start; i <= end; i += 4) {
		output.push_back(std::to_string(i));
	}
	return output;
}
