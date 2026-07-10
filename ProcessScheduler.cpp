#include "ProcessScheduler.h"
#include <iostream>

//makes a unique Worker for each core
ProcessScheduler::ProcessScheduler(int cores, string schedulerType, int quantumCycles, int delayPerExec, int overallMemory, int memPerFrame, int memPerProc)
	: numCores{ cores }, schedulerType{ schedulerType }, quantumCycles{ quantumCycles }, delay{ delayPerExec }, overallMemory{ overallMemory }, memPerFrame{ memPerFrame }, memPerProc{ memPerProc } {
	
	bool isRR = true;
	if (schedulerType == "rr"){
		for (int i = 0 ; i < cores ; i++){
			workers.push_back(std::make_unique<Worker>(i, quantumCycles, isRR, delay));
		}
	}

	else{
		for (int i = 0 ; i < cores ; i++){
			workers.push_back(std::make_unique<Worker>(i, quantumCycles, !isRR, delay));
		}
	}

	memoryManager = std::make_unique<MemoryManager>(overallMemory, memPerFrame, memPerProc);

}

//add process and set it to ready
void ProcessScheduler::addProcess(std::shared_ptr<Process> process, int core)
{
	if (core >= 0 && core < numCores) {
		workers[core]->addProcess(process);
		process->setProcessState(Process::READY);
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
		auto p = worker->getCurrentProcess();

        if (p && p->getState() == Process::RUNNING)
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
