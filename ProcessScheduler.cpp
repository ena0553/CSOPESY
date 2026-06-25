#include "ProcessScheduler.h"
#include <iostream>

//makes a unique Worker for each core
ProcessScheduler::ProcessScheduler(int cores, string schedulerType, int quantumCycles)	
	: numCores{ cores }, schedulerType { schedulerType }, quantumCycles { quantumCycles } {
	
	bool isRR = true;
	if (schedulerType == "rr"){
		for (int i = 0 ; i < cores ; i++){
			workers.push_back(std::make_unique<Worker>(i, quantumCycles, isRR));
		}
	}

	else{
		for (int i = 0 ; i < cores ; i++){
			workers.push_back(std::make_unique<Worker>(i, quantumCycles, !isRR));
		}
	}
	
	
}

//add process and set it to waiting
void ProcessScheduler::addProcess(std::shared_ptr<Process> process, int core)
{
	if (core >= 0 && core < numCores) {
		workers[core]->addProcess(process);
		process->setProcessState(Process::WAITING);
	}
	else {
		std::cerr << "Invalid core ID: " << core << std::endl;
	}
}

// start scheduler
void ProcessScheduler::startScheduler()
{
	if(running) return;
	running = true;
	for(auto& w : workers) w->start();
}

// stop everything
void ProcessScheduler::stopScheduler()
{
	if(!running) return;
	running = false;

	for(auto& w : workers) w->stop();
}

int ProcessScheduler::getBusyCores()
{
	int count = 0;

	for (const auto& worker : workers)
	{
		if (worker->getCurrentProcess() != nullptr)
		{
			count++;
		}
	}

	return count;
}

int ProcessScheduler::getnumCores()
{
	return numCores;
}

string ProcessScheduler::getSchedulerType()
{
	return schedulerType;
}
