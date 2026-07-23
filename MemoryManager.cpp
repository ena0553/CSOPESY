#include "MemoryManager.h"
#include "Process.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <fstream>

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

MemoryManager::MemoryManager(long long totalMemory, long long frameSize, long long memPerProc)
    : totalMemory(totalMemory), frameSize(frameSize), memPerProc(memPerProc)
{
    memoryAllocatorType = PAGING;              // declares this allocator's strategy, per IMemoryAllocator
    maximumSize = static_cast<size_t>(totalMemory);
    currentAllocatedSize = 0;

    totalFrames = totalMemory / frameSize;
    framesPerProcess = memPerProc / frameSize;

    frameTable.resize(static_cast<size_t>(totalFrames));
}

// ------------------------------------------------------------------
// Core paging allocate: hands out `size` bytes as a set of frames that
// do NOT need to be contiguous. That's the whole point of paging -- the
// page table stitches together scattered physical frames into one
// logical allocation, so we never need a best-fit/first-fit search over
// contiguous runs the way a flat allocator would.
// ------------------------------------------------------------------
void* MemoryManager::allocateLocked(size_t size, Process* owner)
{
    long long framesNeeded = (static_cast<long long>(size) + frameSize - 1) / frameSize; // ceil division
    if (framesNeeded <= 0) framesNeeded = 1;

    std::vector<int> freeFrames;
    freeFrames.reserve(static_cast<size_t>(framesNeeded));
    for (int i = 0; i < static_cast<int>(frameTable.size()) &&
                    static_cast<long long>(freeFrames.size()) < framesNeeded; i++)
    {
        if (!frameTable[i].occupied) {
            freeFrames.push_back(i);
        }
    }

    if (static_cast<long long>(freeFrames.size()) < framesNeeded) {
        return nullptr; // Not enough free frames anywhere in memory.
        // A fuller implementation would page out a victim process to the
        // backing store here to make room -- see task 4 discussion.
    }

    void* handle = reinterpret_cast<void*>(nextHandleId++);

    std::vector<PageTableEntry> table;
    table.reserve(static_cast<size_t>(framesNeeded));
    for (int i = 0; i < framesNeeded; i++) {
        int frame = freeFrames[static_cast<size_t>(i)];
        frameTable[frame].occupied = true;
        frameTable[frame].owner = owner;
        frameTable[frame].pageNumber = i;

        table.push_back(PageTableEntry{ frame, true });
    }

    pageTables[handle] = std::move(table);
    if (owner) handleOwner[handle] = owner;

    currentAllocatedSize += static_cast<size_t>(framesNeeded * frameSize);
    numPagedIn += framesNeeded; // Task 7: every frame we bring into memory counts as one page-in.

    return handle;
}

void MemoryManager::deallocateLocked(void* handle)
{
    auto it = pageTables.find(handle);
    if (it == pageTables.end()) return; // unknown/foreign handle -- nothing to do

    for (const PageTableEntry& pte : it->second) {
        if (!pte.valid) continue;
        frameTable[pte.frameNumber] = FrameTableEntry{}; // reset to free
        numPagedOut++; // Task 7: every frame we evict/release counts as one page-out.
    }

    currentAllocatedSize -= it->second.size() * static_cast<size_t>(frameSize);

    pageTables.erase(it);
    handleOwner.erase(handle);
}

// ---- IMemoryAllocator interface ----

void* MemoryManager::allocate(size_t size)
{
    std::lock_guard<std::mutex> lock(memMutex);
    return allocateLocked(size, nullptr);
}

void MemoryManager::deallocate(void* ptr)
{
    std::lock_guard<std::mutex> lock(memMutex);

    // If this handle belonged to a process, keep the process<->handle maps consistent.
    auto ownerIt = handleOwner.find(ptr);
    if (ownerIt != handleOwner.end()) {
        processHandle.erase(ownerIt->second);
    }

    deallocateLocked(ptr);
}

std::string MemoryManager::visualizeMemory()
{
    std::lock_guard<std::mutex> lock(memMutex);
    std::ostringstream oss;

    oss << "Total frames: " << totalFrames << " (frame size " << frameSize << ")\n";
    oss << "Allocated: " << currentAllocatedSize << " / " << maximumSize << "\n";
    oss << "-----------------------------------------\n";

    for (size_t i = 0; i < frameTable.size(); i++) {
        const auto& f = frameTable[i];
        oss << "Frame " << i << ": ";
        if (f.occupied) {
            oss << (f.owner ? f.owner->getName() : std::string("(handle)"))
                << " page " << f.pageNumber;
        } else {
            oss << "free";
        }
        oss << "\n";
    }

    return oss.str();
}

// ---- Process-facing convenience wrappers ----

bool MemoryManager::allocate(Process* process)
{
    std::lock_guard<std::mutex> lock(memMutex);

    void* handle = allocateLocked(static_cast<size_t>(process->getMemoryUsage()), process);
    if (!handle) return false;

    processHandle[process] = handle;
    process->setInMemory(true);
    return true;
}

void MemoryManager::deallocate(Process* process)
{
    std::lock_guard<std::mutex> lock(memMutex);

    auto it = processHandle.find(process);
    if (it == processHandle.end()) return; // process was never in memory

    handleOwner.erase(it->second);
    deallocateLocked(it->second);
    processHandle.erase(it);

    process->setInMemory(false);
}

int MemoryManager::getProcessInMemory() const
{
    std::lock_guard<std::mutex> lock(memMutex);
    return static_cast<int>(processHandle.size());
}

// Paging removes EXTERNAL fragmentation by design (frames need not be
// contiguous), so we no longer report it. What paging still has is
// INTERNAL fragmentation: the last frame given to a process is wasted
// space if the process's memory usage isn't an exact multiple of frameSize.
long long MemoryManager::getInternalFragmentation() const
{
    std::lock_guard<std::mutex> lock(memMutex);
    long long waste = 0;
    for (const auto& kv : pageTables) {
        Process* owner = nullptr;
        auto oit = handleOwner.find(kv.first);
        if (oit != handleOwner.end()) owner = oit->second;
        if (!owner) continue;

        long long allocatedBytes = static_cast<long long>(kv.second.size()) * frameSize;
        long long usedBytes = owner->getMemoryUsage();
        if (allocatedBytes > usedBytes) {
            waste += (allocatedBytes - usedBytes);
        }
    }
    return waste;
}

void MemoryManager::printMemoryState() const
{
    std::cout << const_cast<MemoryManager*>(this)->visualizeMemory();
}

void MemoryManager::printToFile(long long quantumCycle) const
{
    std::filesystem::create_directories("snapshots");
    std::ofstream outFile("snapshots\\memory_stamp_" + std::to_string(quantumCycle) + ".txt");
    if (!outFile.is_open()) return;

    outFile << "Timestamp: " << getCurrentTimestamp() << "\n";
    outFile << "Number of processes in memory: " << getProcessInMemory() << "\n";
    outFile << "Internal fragmentation (bytes): " << getInternalFragmentation() << "\n";
    outFile << "Pages paged in (cumulative): " << numPagedIn << "\n";
    outFile << "Pages paged out (cumulative): " << numPagedOut << "\n";
    outFile << "-----------------------------------------\n";
    outFile << const_cast<MemoryManager*>(this)->visualizeMemory();
}