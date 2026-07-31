#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <stdexcept>
#include <optional>
#include <map>
#include <utility>


class Process; // Forward declaration of Process class

class AccessViolation : public std::runtime_error {
    public:
        AccessViolation(uint32_t address) 
        : std::runtime_error("Access violation"), address_(address) {}
        uint32_t address() const {
            return address_;
        }

    private:
        uint32_t address_;
};

/*
old memory block implementation for FFA

struct MemoryBlock {
    Process* process; // Pointer to the process that owns this block
    bool isValid;       // Flag indicating if the block is allocated

    std::vector<uint16_t> data; // Simulated memory data for this block
};*/

struct Frame {
    bool occupied = false;  // unoccupied by default
    Process* owner = nullptr; 
    int pageNumber = -1;    // which page of owner's address space it holds
    long long lastUsedTick = 0; // for LRU
    std::vector<uint16_t> data; // data in the frame
};

class MemoryManager {
    private:
        std::vector<Frame> frames; // Vector of memory blocks
        long long totalMemory; // Total memory size
        long long frameSize; // Size of each frame
        long long globalTick = 0; // for LRU        
        long long totalFrames; // Total number of frames in memory
        long long pagedIn = 0;
        long long pagedOut = 0;

        std::map<std::pair<int, int>, std::vector<uint16_t>> backingStore;

        mutable std::mutex memMutex;

        int findFreeFrame() const;
        int LRUreplacement();
        void loadPage(Process* process, int pageNumber, int frameIndex);
        void writeBackingStore() const;


        public:
        MemoryManager(long long totalMemory, long long frameSize);

        void deallocate(Process* process); // Deallocate memory for a process
        
        void ensurePageResident(Process* process, uint32_t address); // page fault handler

        int getProcessInMemory() const; // Get the number of processes currently in memory
        long long getUsedMemory() const;
        long long getPagedIn() const {
            return pagedIn;
        }
        long long getPagedOut() const {
            return pagedOut;
        }
        long long getFrameSize() const {
            return frameSize;
        }
        long long getTotalMemory() const {
            return totalMemory;
        }

        void printMemoryState() const; // Print the current state of memory

        uint16_t read(Process* process, uint32_t address); // Read a value from memory at a specific address
        void write(Process* process, uint32_t address, uint16_t value); // Write a value to memory at a specific address

};