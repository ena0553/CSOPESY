#include "FCFSScheduler.h"
#include <iostream>
#include <algorithm>

FCFSScheduler::FCFSScheduler(int cores)
	: numCores{ cores }, processQueues(cores) {
}

void FCFSScheduler::addProcess(const Process& process, int core)
{
	if (core >= 0 && core < numCores) {
		processQueues[core].push_back(process);
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
