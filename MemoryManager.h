#pragma once

#include <vector>
#include <memory>
#include <mutex>

class Process; // Forward declaration of Process class

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
        long long memPerProc;
        long long totalFrames; // Total number of frames in memory
        
        mutable std::mutex memMutex;

        public:
        MemoryManager(long long totalMemory, long long frameSize, long long memPerProc);
        bool allocate(Process* process); // Allocate memory for a process
        void deallocate(Process* process); // Deallocate memory for a process
        
        int getProcessInMemory() const; // Get the number of processes currently in memory
        long long getExternalFragmentation() const; // Get the amount of external fragmentation in memory
        void printMemoryState() const; // Print the current state of memory
};