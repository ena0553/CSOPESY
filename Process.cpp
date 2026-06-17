#include "Process.h"
#include <iostream>

// brace initialization for constructor
Process::Process(int pid, const std::string& type, const std::string& name, const std::string& memoryUsage)
	: pid{ pid }, type{ type }, name{ name }, memoryUsage{ memoryUsage }, currentState{ READY },
	remainingInstructions{ totalCommands } {
}

void Process::setProcessState(ProcessState state)
{
	currentState = state;
}

bool Process::isFinished() const
{
	return remainingInstructions == 0;
}

int Process::getRemainingInstructions() const
{
	return remainingInstructions;
}

void Process::executeInstruction()
{
	if (remainingInstructions > 0) {
		remainingInstructions--;
		commandCounter++;
	}

	if (remainingInstructions == 0) {
		currentState = FINISHED;
	}
}

// getters
int Process::getPID() const { return pid; } // process ID
std::string Process::getType() const { return type; } // process type
std::string Process::getName() const { return name; } // process name
std::string Process::getMemoryUsage() const { return memoryUsage; } // GPU memory usage
Process::ProcessState Process::getState() const { return currentState; } // process state from enums

void Process::log(const std::string& message) const {
	std::cout << "Process " << pid << " (" << name << "): " << message << std::endl; // fix this for correct formatting of log messages
}