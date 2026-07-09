#pragma once

#include <vector>
#include <memory>
#include "Process.h"

struct MemoryBlock {
    Process* process; // Pointer to the process that owns this block
    bool isValid;       // Flag indicating if the block is allocated
};

class MemoryManager {
    private:
        std::vector<MemoryBlock> memory; // Vector of memory blocks
        long long totalMemory; // Total memory size
        long long frameSize; // Size of each frame
        long long framesPerProcess; // Number of frames allocated per process

    public:
        MemoryManager(long long totalMemory, long long frameSize, long long framesPerProcess);
        bool allocate(Process* process); // Allocate memory for a process
        void deallocate(Process* process); // Deallocate memory for a process
        bool isAvailable(); // Check if there is available memory
        void printMemoryStatus(); // Print the current memory status


};