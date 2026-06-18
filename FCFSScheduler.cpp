#include "FCFSScheduler.h"
#include <iostream>
#include <memory>

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
	for (int core = 0; core < numCores; ++core)
	{
		while (!processQueues[core].empty())
		{
			std::shared_ptr<Process> currentProcess = processQueues[core].front();
			processQueues[core].pop();

			currentProcess->setCpuCoreID(core);
			currentProcess->setProcessState(Process::RUNNING);

			while (!currentProcess->isFinished())
			{
				currentProcess->executeNextCommand();
			}

			std::cout << "Process: " << currentProcess->getPID() << " completed on core" << core + 1 << ".\n";
		}
	}
}
