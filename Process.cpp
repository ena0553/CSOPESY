#include "Process.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

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

// Constructor
Process::Process(int pid, const std::string& type, const std::string& name, long long memoryUsage, long long frameSize)
    : pid{ pid }, 
    type{ type }, 
    name{ name }, 
    memoryUsage{ memoryUsage }, 
    frameSize{frameSize},
    currentState{ READY }
{
    // capture creation time once at construction (used by screen -ls display)
    creationTime = getCurrentTimestamp();

    // calculate number of pages
    int numPages = static_cast<int>(memoryUsage / frameSize);
    // make numPages number of PTEs
    pageTable.assign(numPages, PageTableEntry{});
}

// Destructor — close the log if it is open
Process::~Process() {

}

// State management
void Process::setProcessState(ProcessState state) {
    currentState.store(state);
}

// Command management
void Process::addCommand(std::shared_ptr<ICommand> command) {
    commandList.push_back(command);
}

void Process::executeNextCommand() {
    if (isFinished()) return;
    commandList[commandIndex]->execute(*this);  
    commandIndex++;
}

bool Process::isFinished() const {
    return commandIndex.load() >= commandList.size() || hasViolated();
}

// Logging
void Process::log(const std::string& message) {
    std::string ts = getCurrentTimestamp();

    std::string log = ts + " Core:" + std::to_string(cpuCoreID) 
	+ " \"" + message + "\"\n";

	logs.push_back(log); // store in logs vector for screen -s
}

void Process::printLogs() const
{
	for (const std::string& log : logs) {
		std::cout << log;
	}
}

void Process::incrementCommandCounter() {
    commandCounter++;
}


// Getters
int Process::getPID() const                     { return pid; }
std::string Process::getType() const            { return type; }
std::string Process::getName() const            { return name; }
long long Process::getMemoryUsage() const     { return memoryUsage; }
Process::ProcessState Process::getState() const { return currentState.load(); }
int Process::getCommandCounter() const          {
    return commandCounter.load();
}
int Process::getTotalCommands() const { 
    int total = 0;
    for (const auto& cmd : commandList) {
        total += cmd->countCommands();
    }

    return total;
}
int Process::getCpuCoreID() const               { return cpuCoreID.load(); }
std::string Process::getCreationTime() const    { return creationTime; }
std::vector<std::string>& Process::getLogs()    { return logs; }
long long Process::getWakeTick() const               { return wakeTick.load(); }
long long Process::getNextAvailableTick() const { return nextAvailableTick; }

// paging getters
std::vector<Process::PageTableEntry>& Process::getPageTable() { 
    return pageTable; 
}

int Process::getNumPages() const {
    return static_cast<int>(pageTable.size());
}



// Setters
void Process::setCpuCoreID(int coreID) {
    cpuCoreID.store(coreID);
}
void Process::setWakeTick(long long tick) {
    wakeTick = tick;
}
void Process::setNextAvailableTick(long long tick) {
    nextAvailableTick = tick;
}
SymbolTable& Process::getSymbolTable() {
    return symbolTable;
}

// paging setters
void Process::setViolation(uint32_t address, const std::string& timestamp){
    violated = true;
    violationAddress = address;
    violationTimestamp = timestamp;
}

bool Process::hasViolated() const{
    return violated;
}
uint32_t Process::getViolationAddress() const{
    return violationAddress;
}
std::string Process::getViolationTimestamp() const{
    return violationTimestamp;
}