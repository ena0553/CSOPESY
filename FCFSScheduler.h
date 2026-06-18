#pragma once
#include <vector>
#include <queue>

#include "Process.h"

class FCFSScheduler
{
public:
	FCFSScheduler(int cores); // constructor

	void addProcess(std::shared_ptr<Process> process, int core = 0); // add a process to queue

	void runScheduler(); // run scheduler

private:
	int numCores; // number of cores
	std::vector<std::queue<std::shared_ptr<Process>>> processQueues; // one queue for each core

};
