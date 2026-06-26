#include "Worker.h"
#include <iostream>
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
            for (auto it = sleepingProcesses.begin(); it != sleepingProcesses.end(); ) {
                auto& process = *it;
                if (tickCounter.load() >= process->getWakeTick()) {
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
            long long startTick = tickCounter.load();

            while(!process->isFinished() && ( (tickCounter - startTick) < quantumCycles)){
                process->executeNextCommand();
                tickCounter++;
                
                //local tick variable that increments for delay stuff
                long long delayStart = tickCounter.load();
                while ((tickCounter.load() - delayStart) < delay) {
                    tickCounter++;
                }

                if(process->getState() == Process::WAITING){
                    
                    if (process->isFinished())
                    {
                        currentProcess = nullptr;
                        break;
                    }

                    std::lock_guard<std::mutex> lock(queueMutex);
                    sleepingProcesses.push_back(process);
                    currentProcess = nullptr;
                    break;
                }
            }

            if(!process->isFinished() && process->getState() != Process::WAITING){
                process->setProcessState(Process::READY);
                std::lock_guard<std::mutex> lock(queueMutex);
                queue.push(process);
                currentProcess = nullptr;
            }
        }

        else{
            while(!process->isFinished()){
                process->executeNextCommand();
                tickCounter++;

                //local tick variable that increments for delay stuff
                long long delayStart = tickCounter.load();
                while ((tickCounter.load() - delayStart) < delay) {
                    tickCounter++;
                }

                if (process->getState() == Process::WAITING) { // handle waiting (sleeping) processes

                    if (process->isFinished())
                    {
                        currentProcess = nullptr;
                        break;
                    }

                    std::lock_guard<std::mutex> lock(queueMutex);
                    sleepingProcesses.push_back(process);
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