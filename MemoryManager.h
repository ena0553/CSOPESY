#pragma once

#include "IMemoryAllocator.h"

#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

class Process; // Forward declaration of Process class

class MemoryManager : public IMemoryAllocator
{
private:
    // ---- Physical memory side: one entry per physical frame ----
    struct FrameTableEntry
    {
        bool occupied = false;
        Process* owner = nullptr;   // which process this frame currently belongs to (for reporting/visualizeMemory)
        int pageNumber = -1;        // which virtual page of that owner this frame is holding
    };

    // ---- Logical memory side: one entry per virtual page of an allocation ----
    struct PageTableEntry
    {
        int frameNumber = -1;
        bool valid = false;
    };

    std::vector<FrameTableEntry> frameTable; // the physical frame table (size = totalFrames)

    // A "handle" is the void* returned by allocate(). It is an opaque key,
    // NOT a real address, since this emulator has no real backing memory.
    // handle -> that allocation's page table (virtual page -> physical frame)
    std::unordered_map<void*, std::vector<PageTableEntry>> pageTables;
    // handle -> owning process (nullptr for allocations not tied to a process)
    std::unordered_map<void*, Process*> handleOwner;
    // reverse lookup so deallocate(Process*) can find that process's handle
    std::unordered_map<Process*, void*> processHandle;

    uintptr_t nextHandleId = 1; // monotonically increasing synthetic handle generator

    long long totalMemory;
    long long frameSize;
    long long memPerProc;
    long long totalFrames;
    long long framesPerProcess;

    // Task 7: paging-activity counters
    long long numPagedIn = 0;
    long long numPagedOut = 0;

    mutable std::mutex memMutex;

    // Internal helpers that assume memMutex is already held by the caller,
    // so allocate(Process*)/deallocate(Process*) can reuse the core logic
    // without deadlocking on a non-recursive mutex.
    void* allocateLocked(size_t size, Process* owner);
    void  deallocateLocked(void* handle);

public:
    MemoryManager(long long totalMemory, long long frameSize, long long memPerProc);

    // ---- IMemoryAllocator interface (paging strategy) ----
    void* allocate(size_t size) override;   // returns an opaque handle, or nullptr if not enough free frames
    void deallocate(void* ptr) override;    // frees every frame referenced by ptr's page table
    std::string visualizeMemory() override; // dumps the current frame table / page tables as text

    // ---- Process-facing convenience wrappers used by the scheduler/Worker ----
    // Kept so the rest of the codebase (ProcessScheduler, Worker, main.cpp)
    // does not need to change for tasks 1 and 2.
    bool allocate(Process* process);
    void deallocate(Process* process);

    int getProcessInMemory() const;                  // number of processes currently resident
    long long getInternalFragmentation() const;       // see task 6 discussion
    long long getNumPagedIn() const { return numPagedIn; }
    long long getNumPagedOut() const { return numPagedOut; }

    void printMemoryState() const;
    void printToFile(long long quantumCycle) const;
};