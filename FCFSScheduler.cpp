#include "FCFSScheduler.h"
#include <iostream>

//makes a unique Worker for each core
FCFSScheduler::FCFSScheduler(int cores)	: numCores{ cores } {
	for (int i = 0 ; i < cores ; i++){
		workers.push_back(std::make_unique<Worker>(i));
	}
}

//add process and set it to waiting
void FCFSScheduler::addProcess(std::shared_ptr<Process> process, int core)
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
void FCFSScheduler::startScheduler()
{
	if(running) return;
	running = true;
	for(auto& w : workers) w->start();
}

// stop everything
void FCFSScheduler::stopScheduler()
{
	if(!running) return;
	running = false;

	for(auto& w : workers) w->stop();
}

