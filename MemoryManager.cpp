#include "MemoryManager.h"
#include "Process.h"

#include <iostream>

MemoryManager::MemoryManager(long long totalMemory, long long frameSize, long long memPerProc)
    : totalMemory(totalMemory), frameSize(frameSize), memPerProc(memPerProc) {
    // Initialize memory blocks
    totalFrames = totalMemory / frameSize;
    framesPerProcess = memPerProc / frameSize;
    memory.resize(totalFrames);
    for (auto& block : memory) {
        block.process = nullptr;
        block.isValid = true; // Initially, all blocks are free
    }
}

bool MemoryManager::allocate(Process* process) {
    for (size_t i = 0; i <= memory.size() - framesPerProcess; ++i) {
        bool canAllocate = true;
        for (size_t j = 0; j < framesPerProcess; ++j) {
            if (!memory[i + j].isValid) {
                canAllocate = false;
                break;
            }
        }
        if (canAllocate) {
            for (size_t j = 0; j < framesPerProcess; ++j) {
                memory[i + j].process = process;
                memory[i + j].isValid = false;
            }
            process->setInMemory(true); // Mark the process as in memory
            process->setStartFrame(static_cast<int>(i)); // Set the starting frame index
            return true; // Allocation successful
        }
    }
    return false; // Not enough contiguous free frames
}

void MemoryManager::deallocate(Process* process) {
    int start = process->getStartFrame();

    if (start == -1) {
        return; // Process is not in memory
    }

    for (size_t i = start; i < start + framesPerProcess && i < memory.size(); ++i) {
        if (memory[i].process == process) {
            memory[i].process = nullptr;
            memory[i].isValid = true;
        }
    }

    process->setInMemory(false); // Mark the process as not in memory
    process->setStartFrame(-1); // Reset the starting frame index
}

int MemoryManager::getProcessInMemory() const {
    int count = 0;
    for (const auto& block : memory) {
        if (!block.isValid && block.process != nullptr) {
            ++count;
        }
    }
    return count / framesPerProcess; // Return the number of processes currently in memory
}

long long MemoryManager::getExternalFragmentation() const {
    size_t currentRun = 0;
    size_t fragmentedFrames = 0;

    for (const auto& block : memory) {
        if (block.isValid) {
            ++currentRun;
        } else {
            if (currentRun > 0 && currentRun < framesPerProcess) {
                fragmentedFrames += currentRun;
            }
            currentRun = 0;
        }
    }

    // Handle a free run at the end of memory
    if (currentRun > 0 && currentRun < framesPerProcess) {
        fragmentedFrames += currentRun;
    }

    return static_cast<long long>(fragmentedFrames * frameSize);
}

void MemoryManager::printMemoryState() const {
    std::cout << "---end--- = " << totalMemory << "\n\n";

    for (size_t i = 0; i < memory.size(); ++i) {
        if (memory[i].isValid || memory[i].process == nullptr) {
            continue; // Skip blocks without a process
        }

        const Process* process = memory[i].process;

        long long startAddress = static_cast<long long>(i) * frameSize;
        long long endAddress = startAddress + memPerProc;
    
        std::cout << endAddress << "\n";
        std::cout << process->getName() << "\n";
        std::cout << startAddress << "\n\n";

        i += framesPerProcess - 1; // Skip to the next process's memory blocks
    }

    std::cout << "---start--- = 0\n";
}