#include "FCFSScheduler.h"
#include <iostream>

FCFSScheduler::FCFSScheduler(int cores)
	: numCores{ cores }, processQueues(cores) {
}

void FCFSScheduler::addProcess(std::shared_ptr<Process> process, int core)
{
	if (core >= 0 && core < numCores) {
		processQueues[core].push(process); // push process to a selected core's queue
	}
	else {
		std::cerr << "Invalid core ID: " << core << std::endl;
	}
}

void FCFSScheduler::runScheduler()
{
	while (!processQueues[0].empty())
	{
		for (int core = 0; core < numCores; ++core)
		{
			if (!processQueues[core].empty())
			{
				Process currentProcess = processQueues[core].back();
				processQueues[core].pop_back();

				while (!currentProcess.isFinished())
				{
					currentProcess.executeNextCommand();
				}

				std::cout << "Process: " << currentProcess.getPID() << " completed on core" << core + 1 << ".\n";
			}
		}
	}
}
