#include "MemoryManager.h"
#include "Process.h"

#include <iostream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <ctime>

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
    : totalMemory(totalMemory), frameSize(frameSize), memPerProc(memPerProc) {
    // Initialize memory blocks
    totalFrames = totalMemory / frameSize;
    framesPerProcess = memPerProc / frameSize;
    memory.resize(totalFrames);
    for (auto& block : memory) {
        block.process = nullptr;
        block.isValid = true; // Initially, all blocks are free
        block.data.resize(frameSize / sizeof(uint16_t), 0); // Initialize data to zero
    }
}

bool MemoryManager::allocate(Process* process) {
    std::lock_guard<std::mutex> lock(memMutex);
    if (framesPerProcess > memory.size()) return false;
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
    std::lock_guard<std::mutex> lock(memMutex);
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

uint16_t MemoryManager::read(Process* process, uint32_t address) const {
    std::lock_guard<std::mutex> lock(memMutex);
    int startFrame = process->getStartFrame();
    if (!process->isInMemory() || startFrame == -1) {
        throw std::runtime_error("Process is not in memory");
    }

    if (address + sizeof(uint16_t) > memPerProc) {
        throw std::out_of_range("Address out of bounds for this process");
    }

    uint32_t frameOffset = address / frameSize;
    uint32_t offsetWithinFrame = address % frameSize;

    if (offsetWithinFrame % sizeof(uint16_t) != 0) {
        throw std::runtime_error("Address is not aligned to 2 bytes");
    }


    uint32_t blockIndex = startFrame + frameOffset;
    if (blockIndex >= memory.size() || memory[blockIndex].process != process) {
        throw std::runtime_error("Invalid memory access");
    }

    return memory[blockIndex].data[offsetWithinFrame / sizeof(uint16_t)];
}

void MemoryManager::write(Process* process, uint32_t address, uint16_t value) {
    std::lock_guard<std::mutex> lock(memMutex);
    int startFrame = process->getStartFrame();
    if (!process->isInMemory() || startFrame == -1) {
        throw std::runtime_error("Process is not in memory");
    }

    if (address + sizeof(uint16_t) > memPerProc) {
        throw std::out_of_range("Address out of bounds for this process");
    }

    uint32_t frameOffset = address / frameSize;
    uint32_t offsetWithinFrame = address % frameSize;

    if (offsetWithinFrame % sizeof(uint16_t) != 0) {
        throw std::runtime_error("Address is not aligned to 2 bytes");
    }

    uint32_t blockIndex = startFrame + frameOffset;
    if (blockIndex >= memory.size() || memory[blockIndex].process != process) {
        throw std::runtime_error("Invalid memory access");
    }

    memory[blockIndex].data[offsetWithinFrame / sizeof(uint16_t)] = value;
}

int MemoryManager::getProcessInMemory() const {
    std::lock_guard<std::mutex> lock(memMutex);
    int count = 0;
    for (const auto& block : memory) {
        if (!block.isValid && block.process != nullptr) {
            ++count;
        }
    }
    return count / framesPerProcess; // Return the number of processes currently in memory
}

long long MemoryManager::getExternalFragmentation() const {
    std::lock_guard<std::mutex> lock(memMutex);
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
    std::lock_guard<std::mutex> lock(memMutex);
    std::cout << "---end--- = " << totalMemory << "\n\n";

    for (int i = static_cast<int>(memory.size()) - framesPerProcess; i >= 0 ; i -= framesPerProcess) {
        if (memory[i].isValid || memory[i].process == nullptr) {
            continue; // Skip blocks without a process
        }

        const Process* process = memory[i].process;

        long long startAddress = static_cast<long long>(i) * frameSize;
        long long endAddress = startAddress + memPerProc;
    
        std::cout << endAddress << "\n";
        std::cout << process->getName() << "\n";
        std::cout << startAddress << "\n\n";

    }

    std::cout << "---start--- = 0\n";
}

void MemoryManager::printToFile(long long quantumCycle) const
{
    std::filesystem::create_directories("snapshots");
	std::ofstream outFile("snapshots\\memory_stamp_" + std::to_string(quantumCycle) + ".txt");

    if (!outFile.is_open()) {
        return;
    }

    outFile << "Timestamp: " << getCurrentTimestamp() << "\n";
    outFile << "Number of processes in memory: " << getProcessInMemory() << "\n";
    outFile << "Total external fragmentation in KB: " << getExternalFragmentation() << "\n";
    
    std::lock_guard<std::mutex> lock(memMutex);
    outFile << "---end--- = " << totalMemory << "\n\n";

    for (int i = static_cast<int>(memory.size()) - framesPerProcess; i >= 0; i -= framesPerProcess) {
        if (memory[i].isValid || memory[i].process == nullptr) {
            continue; // Skip blocks without a process
        }

        const Process* process = memory[i].process;

        long long startAddress = static_cast<long long>(i) * frameSize;
        long long endAddress = startAddress + memPerProc;

        outFile << endAddress << "\n";
        outFile << process->getName() << "\n";
        outFile << startAddress << "\n\n";

    }

    outFile << "---start--- = 0\n";
}
