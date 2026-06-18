#pragma once
#include <vector>
#include <memory>
#include "Worker.h"


class FCFSScheduler
{
public:
	FCFSScheduler(int cores); // constructor

	void addProcess(std::shared_ptr<Process> process, int core = 0); // add a process to queue
	void startScheduler();
	void stopScheduler();


private:
	int numCores;			// number of cores
	bool running = false;	
	std::vector<std::unique_ptr<Worker>> workers;	// workers vector
};
