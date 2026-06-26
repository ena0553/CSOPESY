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

        if (process->getState() == Process::WAITING) {
            if (tickCounter.load() >= process->getWakeTick()) {
                std::lock_guard<std::mutex> lock(queueMutex);
                process->setProcessState(Process::READY);
            }
        }
        if (process->getState() != Process::READY) {
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.push(process); // put back in queue
            continue;
        }
        else {
            std::lock_guard<std::mutex> lock(queueMutex);
            currentProcess = process; // to check if core is busy
            process->setCpuCoreID(coreId);
            process->setProcessState(Process::RUNNING);
        }


        if(isRR){
            long long now = tickCounter.load();
            long long usedQuantum = 0;

            while(!process->isFinished() && usedQuantum < quantumCycles) {
                // If process is not allowed to run yet (delay-per-exec)
                if (tickCounter.load() < process->getNextAvailableTick()) {
                    continue;
                }
                process->executeNextCommand();
                usedQuantum++;

                if(process->getState() == Process::WAITING){
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (process->isFinished())
                    {
                        currentProcess = nullptr;
                        break;
                    }
                    queue.push(process); // put back in queue if waiting (sleeping)
                    currentProcess = nullptr;
                    break;
                }
                process->setNextAvailableTick(tickCounter.load() + delay);
            }

            if(!process->isFinished() && process->getState() != Process::WAITING){
                std::lock_guard<std::mutex> lock(queueMutex);
                process->setProcessState(Process::READY);
                queue.push(process);
                currentProcess = nullptr;
            }
        }

        else{
            while(!process->isFinished()){
                long long now = tickCounter.load();
                if (now < process->getNextAvailableTick()) {
                    continue; // wait until delay-per-exec is done
                }

                process->executeNextCommand();
                process->setNextAvailableTick(now + delay);
                if (process->getState() == Process::WAITING) { // handle waiting (sleeping) processes
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (process->isFinished())
                    {
                        currentProcess = nullptr;
                        break;
                    }

                    queue.push(process); // push back into queue
                    currentProcess = nullptr; // empty since current process is waiting
                    break; // exit the inner loop to check for other processes
                }
            }
        }

        if (process->isFinished()) {
            std::lock_guard<std::mutex> lock(queueMutex);

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

            currentProcess = nullptr; // empty since current process is done
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // simulate time slice
    }
}

std::shared_ptr<Process> Worker::getCurrentProcess() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return currentProcess;
}