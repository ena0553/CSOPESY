#pragma once
#include <string>

class Process
{
public:
	Process(int pid, const std::string& type, const std::string& name, const std::string& memoryUsage); // constructor

	enum ProcessState // process's state
	{
		READY,
		RUNNING,
		FINISHED
	};

	void setProcessState(ProcessState state); // set process state
	bool isFinished() const; // check if process is finished

	// getters
	int getPID() const; // process ID
	std::string getType() const;  // process type
	std::string getName() const; // process name
	std::string getMemoryUsage() const;  // GPU memory usage
	ProcessState getState() const; // process state from enums

private:
	int pid; // process ID
	std::string type; // process type
	std::string name; // process name

	int cpuCoreID = -1; // CPU core ID that process is running on, -1 if not running(?)
	std::string memoryUsage; // GPU memory usage

	ProcessState currentState; // process's state
	int totalCommands = 100; /* 100 print commands for Week 6 - Group Homework - FCFS scheduler in OS emulator
	in our final program this will be the list of ICommands */
	int commandCounter = 0; // index of the current command being executed
};
