#pragma once
#include <vector>
#include <memory>
#include "Worker.h"

using namespace std;

class ProcessScheduler
{
public:
	ProcessScheduler(int cores, std::string schedulerType, int quantumCycles); // constructor

	void addProcess(std::shared_ptr<Process> process, int core = 0); // add a process to queue
	void startScheduler();
	void stopScheduler();
	int getBusyCores();

	// getters
	int getnumCores();
	string getSchedulerType();


private:
	int numCores;			// number of cores
	string schedulerType;
	int quantumCycles;

	bool running = false;	
	std::vector<std::unique_ptr<Worker>> workers;	// workers vector
};
