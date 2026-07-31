#include "MemoryManager.h"
#include "Process.h"

#include <iostream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
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

// make the memory manager
MemoryManager::MemoryManager(long long totalMemory, long long frameSize)
    : totalMemory(totalMemory), frameSize(frameSize) {
    // Initialize memory blocks
    totalFrames = totalMemory / frameSize;
    //resize frames list based frame size
    frames.resize(totalFrames);
    
    for (auto& frame : frames) {
        frame.occupied = false;     //set frames to unoccupied
        frame.owner = nullptr;
        frame.pageNumber = -1;
        frame.lastUsedTick = 0;
        frame.data.assign(frameSize / sizeof(uint16_t), 0);
    }
}

// iterate thru frames to find an unoccupied one
int MemoryManager::findFreeFrame() const{
    for (size_t i = 0 ; i < frames.size() ; ++i){
        if(!frames[i].occupied){
            return static_cast<int>(i);
        }
    }

    return -1;
}

// evicts the least recently used data out of memory/the frame list
int MemoryManager::LRUreplacement(){
    int evicted = 0;

    //checks for LRU frame
    for(size_t i  = 1 ; i < frames.size() ; ++i){
        if(frames[i].lastUsedTick < frames[evicted].lastUsedTick){
            evicted = static_cast<int>(i);
        }
    }

    Frame& f = frames[evicted];
    
    // writes it to the backing store
    backingStore[{f.owner->getPID(), f.pageNumber}]  = f.data;
    writeBackingStore();

    // frame that had a removal is now unoccupied
    f.owner->getPageTable()[f.pageNumber] = Process::PageTableEntry{};
    f.occupied = false;
    f.owner = nullptr;
    f.pageNumber = -1;
    ++pagedOut;

    return evicted;
}

// loads a page from the backingstore
void MemoryManager::loadPage(Process* process, int pageNum, int frameIndex){
    Frame& f = frames[frameIndex];
    f.occupied = true;
    f.owner = process;
    f.pageNumber = pageNum;
    f.lastUsedTick = globalTick;

    auto it = backingStore.find({process->getPID(), pageNum});
    if(it != backingStore.end()){
        f.data = it->second;
    }
    else {
        std::fill(f.data.begin(), f.data.end(), 0);
    }

    process->getPageTable()[pageNum] = Process::PageTableEntry{true, frameIndex};
}

void MemoryManager::ensurePageResident(Process* process, uint32_t address){
    std::lock_guard<std::mutex> lock(memMutex);

    //checks if the address is beyond the memory usage allowed
    if (address >= static_cast<uint32_t>(process->getMemoryUsage())){
        throw AccessViolation(address);
    }

    int pageNum = static_cast<int>(address / frameSize);
    auto& pageTable = process->getPageTable();
    auto& pageTableEntry = pageTable[pageNum];

    ++globalTick;

    // checks if given PTE is unoccupied and updates its latest use tick
    if(pageTableEntry.valid) {
        frames[pageTableEntry.frameIndex].lastUsedTick = globalTick;
        return;
    }

    // looks for a free frame, if none, perform page fault
    int freeFrame = findFreeFrame();
    if(freeFrame == -1){
        freeFrame = LRUreplacement();
    }

    // load backing store page
    loadPage(process, pageNum, freeFrame);
    ++pagedIn;
}

// writes to the backing store file
void MemoryManager::writeBackingStore() const {
    std::ofstream outFile("csopesy-backing-store.txt");
    
    if(!outFile.is_open()) return;

    for(const auto& [key, data] : backingStore){
        outFile << "pid=" << key.first << " page=" << key.second << " data=";
        for(size_t i = 0 ; i < data.size() ; ++i){
            outFile << data[i];
            if( i + 1 < data.size()){
                outFile << ",";
            }
        }
        outFile << "\n";
    }
}


void MemoryManager::deallocate(Process* process) {
    std::lock_guard<std::mutex> lock(memMutex);

    bool flushed = false;

    for(auto& pte : process->getPageTable()){
        if(pte.valid){
            Frame& f = frames[pte.frameIndex];

            backingStore[{process->getPID(), f.pageNumber}] = f.data;
            flushed = true;

            frames[pte.frameIndex].occupied = false;
            frames[pte.frameIndex].owner = nullptr;
            frames[pte.frameIndex].pageNumber = -1;
            pte.valid = false;
            pte.frameIndex = -1;
        }
    }

    if (flushed) {
        writeBackingStore();
    }
}

uint16_t MemoryManager::read(Process* process, uint32_t address)  {
    //checks if there is a value at that address
    ensurePageResident(process, address);

    std::lock_guard<std::mutex> lock(memMutex);
    //calculates location
    int pageNum = static_cast<int>(address/frameSize);

    int frameIndex = process->getPageTable()[pageNum].frameIndex;

    uint32_t offset = address % frameSize;

    // returns data found
    return frames[frameIndex].data[offset / sizeof(uint16_t)];
}

void MemoryManager::write(Process* process, uint32_t address, uint16_t value) {
    // check if in memory
    ensurePageResident(process, address);

    std::lock_guard<std::mutex> lock(memMutex);
    //find the address to write in
    int pageNum = static_cast<int>(address/frameSize);
    int frameIndex = process->getPageTable()[pageNum].frameIndex;
    uint32_t offset = address % frameSize;

    frames[frameIndex].data[offset / sizeof(uint16_t)]  = value;
}

int MemoryManager::getProcessInMemory() const {
    std::lock_guard<std::mutex> lock(memMutex);
    std::vector<Process*> inMem;

    for (const auto& f : frames) {
        if(f.occupied && f.owner){
            if(std::find(inMem.begin(), inMem.end(), f.owner) == inMem.end()){
                inMem.push_back(f.owner);
            }

        }
    }
    return static_cast<int>(inMem.size());
}


void MemoryManager::printMemoryState() const {
    std::lock_guard<std::mutex> lock(memMutex);
    std::cout << "---end--- = " << totalMemory << "\n\n";

    for (int i = static_cast<int>(frames.size()) - 1; i >= 0; --i) {
        if (!frames[i].occupied || !frames[i].owner) continue;
        long long startAddr = static_cast<long long>(i) * frameSize;
        long long endAddr = startAddr + frameSize;
        std::cout << endAddr << "\n" << frames[i].owner->getName()
                   << " (page " << frames[i].pageNumber << ")\n"
                   << startAddr << "\n\n";
    }
    std::cout << "---start--- = 0\n";
}