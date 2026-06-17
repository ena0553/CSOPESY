#include "Process.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Helper: get the current time as a formatted string "(MM/DD/YYYY HH:MM:SSAM)"
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Process::Process(int pid, const std::string& type, const std::string& name, const std::string& memoryUsage)
    : pid{ pid }, type{ type }, name{ name }, memoryUsage{ memoryUsage }, currentState{ READY }
{
    // capture creation time once at construction (used by screen -ls display)
    creationTime = getCurrentTimestamp();
}

// ---------------------------------------------------------------------------
// Destructor — close the log file if it is open
// ---------------------------------------------------------------------------
Process::~Process() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

// ---------------------------------------------------------------------------
// State management
// ---------------------------------------------------------------------------
void Process::setProcessState(ProcessState state) {
    currentState = state;
}

// ---------------------------------------------------------------------------
// Command management
// ---------------------------------------------------------------------------
void Process::addCommand(std::shared_ptr<ICommand> command) {
    commandList.push_back(command);
}

void Process::executeNextCommand() {
    if (isFinished()) return;
    commandList[commandCounter]->execute(*this);  // runs PrintCommand::execute -> calls this->log()
    commandCounter++;
}

bool Process::isFinished() const {
    return commandCounter >= static_cast<int>(commandList.size());
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
void Process::openLogFile() {
    logFile.open(name + ".txt", std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
        logFile << "Process name: " << name << "\n";
        logFile << "Logs:\n\n";
    } else {
        std::cerr << "[WARNING] Could not open log file for process: " << name << "\n";
    }
}

void Process::log(const std::string& message) {
    std::string ts = getCurrentTimestamp();

    // write to the process's dedicated text file (HW requirement)
    if (logFile.is_open()) {
        logFile << ts << " Core:" << cpuCoreID
                << " \"" << message << "\"\n";
    }
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
int Process::getPID() const                     { return pid; }
std::string Process::getType() const            { return type; }
std::string Process::getName() const            { return name; }
std::string Process::getMemoryUsage() const     { return memoryUsage; }
Process::ProcessState Process::getState() const { return currentState; }
int Process::getCommandCounter() const          { return commandCounter; }
int Process::getTotalCommands() const           { return static_cast<int>(commandList.size()); }
int Process::getCpuCoreID() const               { return cpuCoreID; }
std::string Process::getCreationTime() const    { return creationTime; }

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void Process::setCpuCoreID(int coreID) {
    cpuCoreID = coreID;
}