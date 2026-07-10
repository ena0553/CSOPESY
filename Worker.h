#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

#include "Process.h"
#include "MemoryManager.h"

using namespace std;

class Worker{
    public:
        Worker(int coreId, int quantumCycles, bool isRR, int delay, MemoryManager* memManager);
        ~Worker();

        void start();
        void stop();

        void addProcess(std::shared_ptr<Process>process);
        bool isRunning() const { return running; }

		// getters
        int getCoreId() const { return coreId; }
		std::shared_ptr<Process> getCurrentProcess();


        // Makes a min-heap (smallest wakeTick = highest priority)
        struct WakeCompare {
            bool operator()(const std::shared_ptr<Process>& a,
                            const std::shared_ptr<Process>& b) const
            {
                return a->getWakeTick() > b->getWakeTick();
            }
        };

    private:
        void run();
        
        int coreId;
        int quantumCycles;
        bool isRR;
        int delay;
        
        MemoryManager* memManager;

        std::queue<std::shared_ptr<Process>> queue; // core's respective ready queue
        std::priority_queue<
            std::shared_ptr<Process>,
            std::vector<std::shared_ptr<Process>>,
            WakeCompare
        > sleepingProcesses;                        // priority queue for sleeping processes (fastest waking first)
        
        std::atomic<bool>running = false;
        std::thread coreThread;                     // core's respective thread
        std::shared_ptr<Process> currentProcess = nullptr; // current process running on this core
        std::mutex queueMutex;                      // mutex to prevent race condition
        std::mutex currentProcessMutex;     
        std::mutex sleepingProcessesMutex;    
        
        
};