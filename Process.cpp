#include "Process.h"
#include <iostream>

// brace initialization for constructor
Process::Process(int pid, const std::string& type, const std::string& name, const std::string& memoryUsage)
	: pid{ pid }, type{ type }, name{ name }, memoryUsage{ memoryUsage }, currentState{ READY } {
}

void Process::setProcessState(ProcessState state)
{
	currentState = state;
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