#pragma once
#include <vector>

#include "Process.h"

class FCFSScheduler
{
public:
	FCFSScheduler(int cores); // constructor

	void addProcess(const Process& process, int core = 0); // add a process to queue
	void sortProcessQueues(); // sort process queue based on remaining instructions (FCFS)

	void runScheduler(); // run scheduler

private:
	int numCores; // number of cores
	std::vector<std::vector<Process>> processQueues; // one queue for each core

};
