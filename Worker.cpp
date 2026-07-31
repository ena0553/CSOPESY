#include "Worker.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

extern atomic<long long> tickCounter;
extern vector<shared_ptr<Process>> processList;
extern mutex processListMutex;
extern vector<shared_ptr<Process>> finishedProcessList;
extern mutex finishedProcessListMutex;

Worker::Worker(int coreId, int quantumCycles, bool isRR, int delay, MemoryManager* memManager) 
    : coreId(coreId), quantumCycles(quantumCycles), isRR(isRR), delay(delay), memManager(memManager) {}

Worker::~Worker() { stop(); }

// Helper: get the current time as a formatted string "(MM/DD/YYYY HH:MM:SSAM)"
static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "(%m/%d/%Y %I:%M:%S%p)");
    return oss.str();
}

// start the thread
void Worker::start(){
    if (running) return;
    running = true;
    coreThread = thread(&Worker::run, this);
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
void Worker::addProcess(shared_ptr<Process> process){
    lock_guard<mutex> lock(queueMutex);
    process->setProcessState(Process::READY);
    queue.push(process);
}

// run the process per instruction
void Worker::run(){
    while(running){

        // Wake sleeping processes
        {
            lock_guard<mutex> lock(queueMutex);
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
        shared_ptr<Process> process = nullptr;
        {
            lock_guard<mutex> lock(queueMutex);
            if(!queue.empty()){
                process = queue.front();
                queue.pop();
            }
        }

        if(!process){
            this_thread::sleep_for(chrono::milliseconds(10)); // idle wait
            continue;
        }
        
        // check if the process is in memory, if not, attempt to allocate it

        {
            lock_guard<mutex> lock(currentProcessMutex);
            currentProcess = process; 
        }
        {
            lock_guard<mutex> lock(queueMutex);
            process->setCpuCoreID(coreId);
            process->setProcessState(Process::RUNNING);
        }
           
        


        if(isRR){
            long long quantumStart = tickCounter.load();

            while(!process->isFinished()) {
                // If process is not allowed to run yet (delay-per-exec)
                auto now = tickCounter.load();

                // Quantum expired
                if (now - quantumStart >= quantumCycles) {
                    break;
                }

                // Busy wait (delay-per-exec)
                if (now < process->getNextAvailableTick()) {
                    continue;
                }
                try{
                    process->executeNextCommand();
                } catch(const AccessViolation& e){
                    process->setViolation(e.address(), getCurrentTimestamp());
                    process->setProcessState(Process::TERMINATED);
                    memManager->deallocate(process.get());

                    {
                        lock_guard<mutex> lock(finishedProcessListMutex);
                        finishedProcessList.push_back(process);
                    }
                    {
                        lock_guard<mutex> lock(processListMutex);
                        processList.erase(remove(processList.begin(), processList.end(), process), processList.end());
                    }
                    {
                        lock_guard<mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                    }
                    break;
                }

                if(process->getState() == Process::WAITING) { // sleep command
                    if (process->isFinished())
                     {
                        lock_guard<mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                        break;
                    }

                    {
                        lock_guard<mutex> lock(sleepingProcessesMutex);
                        sleepingProcesses.push(process); // push into sleeping queue
                    }
                    {
                        lock_guard<mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                    }
                    break;
                }
                process->setNextAvailableTick(now + delay);
            }

            // If a process is not yet finished and is not sleeping, put it back into the ready queue after its time slice
            if(!process->isFinished() && process->getState() != Process::WAITING){
                lock_guard<mutex> lock1(queueMutex);
                lock_guard<mutex> lock2(currentProcessMutex);
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

                try{
                    process->executeNextCommand();
                } catch (const AccessViolation& e){
                    process->setViolation(e.address(), getCurrentTimestamp());
                    process->setProcessState(Process::TERMINATED);
                    memManager->deallocate(process.get());

                    {
                        lock_guard<mutex> lock(finishedProcessListMutex);
                        finishedProcessList.push_back(process);
                    }
                    {
                        lock_guard<mutex> lock(processListMutex);
                        processList.erase(remove(processList.begin(), processList.end(), process), processList.end());
                    }
                    {
                        lock_guard<mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                    }
                    break;
                }

                process->setNextAvailableTick(now + delay);
                if (process->getState() == Process::WAITING) { // handle waiting (sleeping) processes
                    if (process->isFinished())
                    {
                        lock_guard<mutex> lock(currentProcessMutex);
                        currentProcess = nullptr;
                        break;
                    }

                    {
                        lock_guard<mutex> lock(sleepingProcessesMutex);
                        sleepingProcesses.push(process); // push into sleeping queue
                    }
                    {
                        lock_guard<mutex> lock(currentProcessMutex);
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
            memManager->deallocate(process.get());  // deallocate when the process is done

            // add to finished vector
            {
                lock_guard<mutex> lock(finishedProcessListMutex);
                finishedProcessList.push_back(process);
            }
            
            {
                lock_guard<mutex> lock(processListMutex);
                processList.erase(remove(processList.begin(), processList.end(), process), processList.end());
            }
            {
                lock_guard<mutex> lock(currentProcessMutex);
                currentProcess = nullptr; // empty since current process is done
            }
           
            
        }
    }
}

shared_ptr<Process> Worker::getCurrentProcess() {
    lock_guard<mutex> lock(currentProcessMutex);
    return currentProcess;    
}