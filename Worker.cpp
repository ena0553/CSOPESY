#include "Worker.h"
#include <iostream>
#include <algorithm>

extern std::atomic<long long> tickCounter;
extern vector<shared_ptr<Process>> processList;
extern mutex processListMutex;
extern vector<shared_ptr<Process>> finishedProcessList;
extern mutex finishedProcessListMutex;

Worker::Worker(int coreId, int quantumCycles, bool isRR, int delay) : coreId(coreId), quantumCycles(quantumCycles), isRR(isRR), delay(delay) {}

Worker::~Worker() { stop(); }

// start the thread
void Worker::start(){
    if (running) return;
    running = true;
    coreThread = std::thread(&Worker::run, this);
}

// stop the thread
void Worker::stop(){
    if(!running) return;
    running = false;
    if(coreThread.joinable()){
        coreThread.join();
    }
}

// add a process to the Worker's RQ
void Worker::addProcess(std::shared_ptr<Process> process){
    std::lock_guard<std::mutex> lock(queueMutex);
    process->setProcessState(Process::READY);
    queue.push(process);
}

// run the process per instruction
void Worker::run(){
    while(running){

        // Wake sleeping processes
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!sleepingProcesses.empty()) {
                auto process = sleepingProcesses.top();

                // not ready yet → stop immediately (important!)
                if (tickCounter.load() < process->getWakeTick())
                    break;

                sleepingProcesses.pop();

                process->setProcessState(Process::READY);
                queue.push(process);
            }
        }
        std::shared_ptr<Process> process = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if(!queue.empty()){
                process = queue.front();
                queue.pop();
            }
        }

        if(!process){
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // idle wait
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(currentProcessMutex);
            currentProcess = process; 
        }
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            process->setCpuCoreID(coreId);
            process->setProcessState(Process::RUNNING);
        }
           
        


        if(isRR){
            long long usedQuantum = 0;

            while(!process->isFinished() && usedQuantum < quantumCycles) {
                // If process is not allowed to run yet (delay-per-exec)
                auto now = tickCounter.load();
                if (now < process->getNextAvailableTick()) {
                    continue;
                }
                process->executeNextCommand();
                usedQuantum++;

                if(process->getState() == Process::WAITING) { // sleep command
                    if (process->isFinished())
                     {
                        std::lock_guard<std::mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                        break;
                    }

                    {
                        std::lock_guard<std::mutex> lock(sleepingProcessesMutex);
                        sleepingProcesses.push(process); // push into sleeping queue
                    }
                    {
                        std::lock_guard<std::mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                    }
                    break;
                }
                process->setNextAvailableTick(now + delay);
            }

            // If a process is not yet finished and is not sleeping, put it back into the ready queue after its time slice
            if(!process->isFinished() && process->getState() != Process::WAITING){
                std::lock_guard<std::mutex> lock1(queueMutex);
                std::lock_guard<std::mutex> lock2(currentProcessMutex);
                process->setProcessState(Process::READY);
                queue.push(process); // push back into ready queue
                currentProcess = nullptr;
            }
        }

        else{
            while(!process->isFinished()){
                auto now = tickCounter.load();
                if (now < process->getNextAvailableTick()) {
                    continue; // wait until delay-per-exec is done
                }

                process->executeNextCommand();
                process->setNextAvailableTick(now + delay);
                if (process->getState() == Process::WAITING) { // handle waiting (sleeping) processes
                    if (process->isFinished())
                    {
                        std::lock_guard<std::mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                        break;
                    }

                    {
                        std::lock_guard<std::mutex> lock(sleepingProcessesMutex);
                        sleepingProcesses.push(process); // push into sleeping queue
                    }
                    {
                        std::lock_guard<std::mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                    }
                    
                    break; // exit the inner loop to check for other processes
                }
            }
        }

        if (process->isFinished()) {
            if (process->getState() == Process::TERMINATED) {
                continue;
            }
            process->setProcessState(Process::TERMINATED); // make sure process ends

            // add to finished vector
            {
                std::lock_guard<std::mutex> lock(finishedProcessListMutex);
                finishedProcessList.push_back(process);
            }
            
            {
                std::lock_guard<std::mutex> lock(processListMutex);
                processList.erase(remove(processList.begin(), processList.end(), process), processList.end());
            }
            {
                std::lock_guard<std::mutex> lock(currentProcessMutex);
                currentProcess = nullptr; // empty since current process is done
            }
           
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // simulate time slice
    }
}

std::shared_ptr<Process> Worker::getCurrentProcess() {
    std::lock_guard<std::mutex> lock(currentProcessMutex);
    return currentProcess;    
}