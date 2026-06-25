#include "Worker.h"
#include <iostream>
extern std::atomic<long long> globalTick;

Worker::Worker(int coreId, int quantumCycles, bool isRR) : coreId(coreId), quantumCycles(quantumCycles), isRR(isRR) {}

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
            for (auto it = sleepingProcesses.begin(); it != sleepingProcesses.end(); ) {
                auto& process = *it;
                process->decrementSleepTicks();
                if (process->getSleepTicks() <= 0) {
                    process->setProcessState(Process::READY);
                    queue.push(process);
                    it = sleepingProcesses.erase(it); // remove from sleepingProcesses
                } else {
                    ++it;
                }
            }
        }
        std::shared_ptr<Process> process = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if(!queue.empty()){
                process = queue.front();
                queue.pop();
                if (process) {
                    currentProcess = process; // to check if core is busy
                    process->setCpuCoreID(coreId);
                    process->setProcessState(Process::RUNNING);
                }
            }
        }

        if(!process){
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // idle wait
            continue;
        }

        
        


        if(isRR){
            
        }

        else{
            while(!process->isFinished()){
                process->executeNextCommand();
                if (process->getState() == Process::WAITING) { // handle waiting (sleeping) processes
                    std::lock_guard<std::mutex> lock(queueMutex);
                    sleepingProcesses.push_back(process);
                    currentProcess = nullptr; // empty since current process is waiting
                    break; // exit the inner loop to check for other processes
                }
            }
        }

        if (process->isFinished()) {
            std::lock_guard<std::mutex> lock(queueMutex);
            process->setProcessState(Process::TERMINATED); // make sure process ends   
            currentProcess = nullptr; // empty since current process is done
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // simulate time slice
    }
}

std::shared_ptr<Process> Worker::getCurrentProcess() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return currentProcess;
}