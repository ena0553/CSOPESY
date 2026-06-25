#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include "Process.h"
#include <atomic>

using namespace std;

class Worker{
    public:
        Worker(int coreId, int quantumCycles, bool isRR, int delay);
        ~Worker();

        void start();
        void stop();

        void addProcess(std::shared_ptr<Process>process);
        bool isRunning() const { return running; }

		// getters
        int getCoreId() const { return coreId; }
		std::shared_ptr<Process> getCurrentProcess();

    private:
        void run();
        
        int coreId;
        int quantumCycles;
        bool isRR;
        int delay;
        
        std::queue<std::shared_ptr<Process>> queue; // core's respective queue
        std::vector<std::shared_ptr<Process>> sleepingProcesses; // completed processes for this core
        std::mutex queueMutex;                      // mutex to prevent race condition
        std::atomic<bool>running = false;
        std::thread coreThread;                     // core's respective thread
		std::shared_ptr<Process> currentProcess = nullptr; // current process running on this core
};