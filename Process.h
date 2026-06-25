#pragma once

#include "commands/ICommand.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <atomic>

class Process
{
public:
    Process(int pid, const std::string& type, const std::string& name, const std::string& memoryUsage); // constructor
    ~Process(); // destructor to close log file

    enum ProcessState // process's state
    {
        READY,
        RUNNING,
        WAITING,
        TERMINATED,
    };

    // --- State management ---
    void setProcessState(ProcessState state);

    // --- Command management ---
    void addCommand(std::shared_ptr<ICommand> command); // add a command to the list
    void executeNextCommand();                           // execute the next command in the list
    bool isFinished() const;                            // true when all commands have been executed

    // --- Logging ---
    void openLogFile();                                          // open the process's .txt log file
    void log(const std::string& message);                       // write a timestamped log entry
    void printLogs() const;

    // --- Getters ---
    int getPID() const;
    std::string getType() const;
    std::string getName() const;
    std::string getMemoryUsage() const;
    ProcessState getState() const;
    int getCommandCounter() const;    // number of commands executed so far
    int getTotalCommands() const;     // total number of commands
    int getCpuCoreID() const;         // which core is running this process
    std::string getCreationTime() const;  // timestamp string for screen -ls display
	std::vector<std::string>& getLogs(); // for screen -s to access logs
    long long getWakeTick() const; // get the remaining sleep ticks
    SymbolTable& getSymbolTable(); // access this process's variable store

    // --- Setters ---
    void setCpuCoreID(int coreID);
    void setWakeTick(long long tick); // set the remaining sleep ticks

private:
    int pid;                    // process ID
    std::string type;           // process type (e.g. "screen")
    std::string name;           // process name
    std::string memoryUsage;    // memory usage label
    std::string creationTime;   // timestamp captured at construction (for screen -ls display)

    typedef std::vector<std::shared_ptr<ICommand>> CommandList;
    CommandList commandList;    // list of commands to execute

    std::atomic<int> cpuCoreID = -1;         // CPU core ID running this process; -1 if not yet assigned
    std::atomic<int> commandCounter = 0;     // index of the next command to execute

    std::atomic<ProcessState> currentState;  // current state of this process

    std::ofstream logFile;      // file stream for this process's .txt log
    std::vector<std::string> logs;

    std::atomic<long long> wakeTick = 0;
    SymbolTable symbolTable;     // per-process variable memory
};
