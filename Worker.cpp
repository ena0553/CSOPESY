#include "Worker.h"
#include <iostream>

Worker::Worker(int coreId) : coreId(coreId) {}

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
    process->setProcessState(Process::WAITING);
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

                currentProcess = process; // to check if core is busy
            }
        }

        if(!process){
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // idle wait
            continue;
        }

        process->setCpuCoreID(coreId);
        process->setProcessState(Process::RUNNING);
        process->openLogFile(); // prints onto the text files

        while(!process->isFinished()){
            process->executeNextCommand();
        }

        process->setProcessState(Process::TERMINATED); // make sure process ends
		currentProcess = nullptr; // empty since current process is done
        std::cout << "Core" << coreId << "finished process " << process->getPID() << std::endl;
    }
}
