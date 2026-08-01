#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <iomanip>
#include <random>
#include <chrono>
#include <sstream>

#include "ProcessScheduler.h"
#include "Process.h"
#include "CommandParser.h"
#include "commands/PrintCommand.h"
#include "commands/SleepCommand.h"
#include "commands/SubtractCommand.h"
#include "commands/ForCommand.h"
#include "commands/DeclareCommand.h"
#include "commands/AddCommand.h"
#include "commands/MemoryCommand.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <thread>

#include <fstream>

using namespace std;

// Global process list 
mutex processListMutex;
vector<shared_ptr<Process>> processList;
atomic<int> pidCounter = 1; // global PID counter unique PID assignment for processes

// finished process list (completed strings vector)
mutex finishedProcessListMutex;
vector<shared_ptr<Process>> finishedProcessList;

// global tick counter
atomic<long long> tickCounter = 0;
atomic<bool> cpuRunning = false;
thread tickThread;

// cpu tick counters
atomic<long long> activeCpuTicks = 0;
atomic<long long> idleCpuTicks = 0;

// global generator thread for generating process in scheduler-start
atomic<bool> generating = false;
thread generatorThread;

static int varId = 1; // global variable counter

// config structure for config file variables
struct Config
{
    int numCpu; // range 1 - 128
    string scheduler; // fcfs or rr
    long long quantumCycles; // range 1 - 2^32 (4294967296)
    long long batchProcessFreq; // range 1 - 2^32 (4294967296)
    long long minIns; // range 1 - 2^32 (4294967296)
    long long maxIns; // range 1 - 2^32 (4294967296)
    long long delayPerExec; // range 0 - 2^32 (4294967296)
	long long maxOverallMem; // range 2^6 (64) - 2^16 (65536)
    long long memPerFrame; // range 2^6 (64) - 2^16 (65536)
    long long minMemPerProc; // range 2^6 (64) - 2^16 (65536)
    long long maxMemPerProc; // range 2^6 (64) - 2^16 (65536)
};

enum class Mode {
    MAIN,
    SUBSCREEN
};

// To see how many executed instructions a command will represent
struct GeneratedCommand {
    shared_ptr<ICommand> command;
    int instructionCount;
};

// Header
void displayHeader() {
    cout << " _____ _____ _____ _____ _____ _____ __ __ \n";
    cout << "|     |   __|     |  _  |   __|   __|  |  |\n";
    cout << "|   --|__   |  |  |   __|   __|__   |_   _|\n";
    cout << "|_____|_____|_____|__|  |_____|_____| |_|  \n";

    cout << "Hello, Welcome to CSOPESY commandline!" << endl;
    cout << "Type 'exit' to quit, 'help' to display available commands, 'clear' to clear the screen.\n" << endl;
    cout << "** IMPORTANT: Type 'initialize' to load config and start system **" << endl;
}

// screen -ls display
void screen_ls(ProcessScheduler& scheduler) {
    int used = scheduler.getBusyCores();
    int totalCores = scheduler.getnumCores();
    
    cout << "CPU Utilization: " << (used * 100.0 / totalCores) << endl;
    cout << "Cores used: " << used << endl;
    cout << "Cores available: " << (totalCores - used) << endl;

    cout << "---------------------------------------\n";
    cout << "Running processes:\n";
    {   // lock when reading processList to avoid sync issues
        lock_guard<mutex> lock(processListMutex); 
        for (auto& p : processList) {
        if (p->getState() == Process::RUNNING) {
            cout << left << setw(12) << p->getName()
                << " " << p->getCreationTime()
                << "   Core: " << p->getCpuCoreID()
                << "   " << p->getCommandCounter()
                << " / " << p->getTotalCommands()
                << "\n";
            }
        }
    }
    cout << "\nFinished processes:\n";
    { // lock when reading finishedProcessList to avoid sync issues
        lock_guard<mutex> lock(finishedProcessListMutex);
        for (auto& p : finishedProcessList) {
            cout << left << setw(12) << p->getName()
                << " " << p->getCreationTime()
                << "   Finished"
                << "   " << p->getTotalCommands()
                << " / " << p->getTotalCommands()
                << "\n";
        }
    }
    
    cout << "---------------------------------------\n";
}

void report_util(ProcessScheduler& scheduler) {
    ofstream outFile("report-util.txt");

    int used = scheduler.getBusyCores();
    int totalCores = scheduler.getnumCores();

    outFile << "CPU Utilization: " << (used * 100.0 / totalCores) << endl;
    outFile << "Cores used: " << used << endl;
    outFile << "Cores available: " << (totalCores - used) << endl;

    outFile << "---------------------------------------\n";
    outFile << "Running processes:\n";
    {   // lock when reading to avoid sync issues
        lock_guard<mutex> lock(processListMutex);
        for (auto& p : processList) {
        if (p->getState() == Process::RUNNING) {
            outFile << left << setw(12) << p->getName()
                << " " << p->getCreationTime()
                << "   Core: " << p->getCpuCoreID()
                << "   " << p->getCommandCounter()
                << " / " << p->getTotalCommands()
                << "\n";
        }
    }
    
    }

    outFile << "\nFinished processes:\n";
    // lock when reading finishedProcessList to avoid sync issues
    lock_guard<mutex> lock(finishedProcessListMutex);
    for (auto& p : finishedProcessList) {
        outFile << left << setw(12) << p->getName()
            << " " << p->getCreationTime()
            << "   Finished"
            << "   " << p->getTotalCommands()
            << " / " << p->getTotalCommands()
            << "\n";
    }
    outFile << "---------------------------------------\n";
}

// process-smi command for screen -s
void process_smi(ProcessScheduler& scheduler, shared_ptr<Process>& process)
{
    cout << "Process name: " << process->getName() << endl;
    cout << "PID: " << process->getPID() << endl;
    cout << "Logs: " << endl;
    process->printLogs();
    cout << "\n" << "Current instruction line: " << process->getCommandCounter() << endl;
    cout << "Lines of code: " << process->getTotalCommands() << endl;
    if (process->isFinished())
    {
        cout << "Finished!" << endl;
    }

}

void mainMenu_process_smi(MemoryManager* memManager, ProcessScheduler& scheduler)
{
    int used = scheduler.getBusyCores();
    int totalCores = scheduler.getnumCores();

    cout << "----------------------------------------------\n";
    cout << "| PROCESS-SMI V01.00 Driver Version: 01.00 |\n";
    cout << "----------------------------------------------\n";
    cout << "CPU-Util: " << (used * 100.0 / totalCores) << "%\n";
    cout << "Memory Usage: " << memManager->getUsedMemory() / (1048576.0) << "MiB / " << memManager->getTotalMemory() / (1048576.0) << "MiB\n"; // FIXME: might need MiB?
	cout << "Memory Util: " << (memManager->getUsedMemory() * 100.0 / memManager->getTotalMemory()) << "%\n\n";

    cout << "========================================\n";
    cout << "Running processes and memory usage:  \n";
    cout << "----------------------------------------\n";
    {   // lock when reading processList to avoid sync issues
        lock_guard<mutex> lock(processListMutex);
        for (auto& p : processList) {
            if (p->getState() == Process::RUNNING) {
                cout << left << setw(12) << p->getName()
                    << " " << p->getMemoryUsage() / (1048576.0)
                    << "MiB\n";
            }
        }
    }
    cout << "----------------------------------------\n";
}

// vmstat command
void vmstat(MemoryManager* memManager = nullptr) // FIXME: might need units
{
    cout << memManager->getTotalMemory() << " total memory" << endl;
	cout << memManager->getUsedMemory() << " used memory" << endl;
	cout << memManager->getFreeMemory() << " free memory" << endl;
	cout << idleCpuTicks.load() << " idle cpu ticks" << endl; // not sure if it means all cores are idle at a time? or per core? will probably need changing
	cout << activeCpuTicks.load() << " active cpu ticks" << endl; // is counter per core
	cout << idleCpuTicks.load() + activeCpuTicks.load() << " total cpu ticks" << endl; // sum of prev two. not sure if same as global tick counter?
	cout << memManager->getPagedIn() << " pages paged in" << endl;
    cout << memManager->getPagedOut() << " pages paged out" << endl;
}

// Helper: get a random integer between minimum instructions and maximum instructions
int getRandomInt(int minIns, int maxIns)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(minIns, maxIns);
    return dist(gen);
}

// Helper: get a random integer between minimum memory per process and maximum memory per process
int getRandomPowerOfTwo(int minMemPerProc, int maxMemPerProc)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    int minExp = std::log2(minMemPerProc); // exponent
    int maxExp = std::log2(maxMemPerProc); // exponent

    std::uniform_int_distribution<int> dist(minExp, maxExp); // choose random exponent

    return 1 << dist(gen); // returns 2^(random exponent)
}

// Helper: check if number is a power of two
bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Helper: Generate two operands (for add and subtract commands)
std::pair<Operand, Operand> generateOperands() {
    Operand op1;
    Operand op2;
    // will declare a new variable with value 0 if empty, varId + 1 to have a chance to choose a non-existing variable
    if (getRandomInt(1, 2) == 1) {
        op1 = std::string("var") + std::to_string(getRandomInt(1, varId + 1)); 
    } else {
        op1 = static_cast<uint16_t>(getRandomInt(0, 500));
    }

    if (getRandomInt(1, 2) == 1) {
        op2 = std::string("var") + std::to_string(getRandomInt(1, varId + 1));
    } else {
        op2 = static_cast<uint16_t>(getRandomInt(0, 500));
    }

    return {op1, op2};
}

uint32_t generateRandomAddress(long long memPerProc) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Ensure the address is aligned to 2 bytes
    std::uniform_int_distribution<uint32_t> dist(0, static_cast<uint32_t>(memPerProc / sizeof(uint16_t) - 1)); 
    return dist(gen)  * sizeof(uint16_t); 
}

// Generate random commands for a process. For loops can be nested up to 3 times
GeneratedCommand generateRandomCommand(const string& name, int depth, int& remainingCommands, long long memoryUsage, MemoryManager* memManager = nullptr) {
    // Only allow nesting up to 3 times and allow FOR command if there is enough remaining commands for there to be instructions in the FOR
    int maxType = (depth < 3 && remainingCommands > 2) ? 8 : 7;
    int type = getRandomInt(1, maxType); // 1: Print, 2: Declare, 3: Add, 4: Subtract, 5: Sleep, 6: Read, 7: Write, 8: For (only if depth < 3 and remainingCommands > 2)
    switch (type) {
        case 1: {
            // PRINT: unless specified in the test case, the “msg” should always be “Hello world from <process_name>!”
            string msg = "Hello world from " + name + "!";
            remainingCommands--;
            return {make_shared<PrintCommand>(msg), 1};
        }
        case 2: {
            // DECLARE: random variable name, random initial value
            std::string varName = "var" + to_string(varId++);
            uint16_t initVal = static_cast<uint16_t>(getRandomInt(0, 1000));
            remainingCommands--;
            return {make_shared<DeclareCommand>(varName, initVal), 1};
        }
        case 3: {
            // ADD: performs an addition operation: var1 = var2/value + var3/value
            auto [op1, op2] = generateOperands();
            std::string dest  = "var" + to_string(varId++);
            remainingCommands--;
            return {make_shared<AddCommand>(dest, op1, op2), 1};
            
        }
        case 4: {
            // SUBTRACT: performs a subtraction operation: var1 = var2/value - var3/value
            auto [op1, op2] = generateOperands();
            std::string dest  = "var" + to_string(varId++);
            remainingCommands--;
            return {make_shared<SubtractCommand>(dest, op1, op2), 1};
        }
        case 5: {
            // SLEEP: random sleep ticks between 1 and 5
            uint8_t ticks = getRandomInt(1, 5); // Random sleep ticks between 1 and 5
            remainingCommands--;
            return {make_shared<SleepCommand>(ticks), 1};
        }
        case 6: {
            // READ: reads a value from memory at a specific address and stores it in a variable
            uint32_t address = generateRandomAddress(memoryUsage);
            std::string varName = "var" + to_string(varId++); // variable to store the result
            remainingCommands--;
            return {make_shared<MemoryCommand>(MemoryCommand::Operation::READ, memManager, address, varName, Operand{}), 1};
        }
        case 7: {
            // WRITE: writes a value to memory at a specific address from a variable or literal
            uint32_t address = generateRandomAddress(memoryUsage);
            Operand source;
            if (getRandomInt(1, 2) == 1) {
                source = std::string("var") + std::to_string(getRandomInt(1, varId - 1)); // random variable 
            } else {
                source = static_cast<uint16_t>(getRandomInt(0, 500)); // random literal value
            }
            remainingCommands--;
            return {make_shared<MemoryCommand>(MemoryCommand::Operation::WRITE, memManager, address, "", source), 1};
        }
        case 8: {
            // FOR: loop [1, 3] random commands for [2, 5] iterations (can be nested up to 3 times) 
            int before = remainingCommands;

            // Try at most 20 times to generate a valid FOR loop within the max instruction count
            for (int tries = 0; tries < 20; tries++) {
                remainingCommands = before;
                int maxBody = min(3, remainingCommands - 1); // Make sure it doesn't exceed the max number of commands
                if (maxBody < 1) break;
                int bodySize = getRandomInt(1, maxBody);
                std::vector<std::shared_ptr<ICommand>> instructions;
                int insCount = 0;
                for (int i = 0; i < bodySize; i++) {
                    GeneratedCommand cmd = generateRandomCommand(name, depth + 1, remainingCommands, memoryUsage, memManager);
                    instructions.push_back(cmd.command);
                    insCount += cmd.instructionCount;
                }
                // Ensure that it does not exceed max instruction count
                if (insCount == 0) continue;

                int maxIter = min(5, (before - 1) / insCount);
                int count;
                if (maxIter < 2) {
                    count = 1;
                } else {
                    count = getRandomInt(2, maxIter);
                }
                 
                int totalIns = 1 + count * insCount;
                if (totalIns <= before) {
                    remainingCommands -= totalIns - insCount;
                    return {make_shared<ForCommand>(count, instructions), totalIns};
                }
            }
            

            // Not enough space for instructions, make a different command
            remainingCommands = before;
            return generateRandomCommand(name, depth, remainingCommands, memoryUsage, memManager);
            
        }
    }
    // this should never be reached
    return {make_shared<PrintCommand>("Error generating command"), 1};
}

// Randomly generates numCommands number of commands for a process
shared_ptr<Process> makeProcess (int pid, const string& name, int numCommands, long long memoryUsage = 0, long long frameSize = 64, MemoryManager* memManager = nullptr) {
    int remaining = numCommands;
    auto p = make_shared<Process>(pid, "screen", name, memoryUsage, frameSize);
    while (remaining > 0) {
        GeneratedCommand cmd = generateRandomCommand(name, 0, remaining, memoryUsage, memManager);
        p->addCommand(cmd.command);
    }
    return p;
}

int processesCreated = 0; // for testing
// Continuously generate dummy processes every X CPU ticks until scheduler-stop is called. Frequency can be set in config.txt
void scheduler_start(ProcessScheduler& scheduler, Config config, MemoryManager* memManager = nullptr) {
    if (generating) {
        cout << "Process generation is already running." << endl;
        return;
    }
    cout << "Generating processes..." << endl;
    generating = true;
    processesCreated = 0;

    generatorThread = std::thread([&scheduler, config, memManager]() {
        int coreID = 0;
        while (generating) {
            int numCommands = getRandomInt(config.minIns, config.maxIns);
            int memPerProc = getRandomPowerOfTwo(config.minMemPerProc, config.maxMemPerProc);
            string name = string("process") + (pidCounter < 10 ? "0" : "") + to_string(pidCounter);
            auto newProcess = makeProcess(pidCounter++, name, numCommands, memPerProc, config.memPerFrame, memManager);
            {
                lock_guard<mutex> lock(processListMutex);
                processList.push_back(newProcess);
                processesCreated++;
            }
            

            // Assign the process to a core in a round-robin fashion
            scheduler.addProcess(newProcess, coreID);
            coreID = (coreID + 1) % scheduler.getnumCores();

            long long startTick = tickCounter.load();
            while (generating && tickCounter.load() - startTick < config.batchProcessFreq) {
                std::this_thread::yield();
            }
        }
    });
}

// this creates the config file if it's not in the folder yet
void createConfigFile()
{
    std::ofstream file("config.txt");

    file << "num-cpu 4\n";
    file << "scheduler \"rr\"\n";
    file << "quantum-cycles 5\n";
    file << "batch-process-freq 1\n";
    file << "min-ins 1000\n";
    file << "max-ins 2000\n";
    file << "delay-per-exec 0\n";
    file << "max-overall-mem 16384\n";
    file << "mem-per-frame 64\n";
    file << "min-mem-per-proc 4096\n";
    file << "max-mem-per-proc 4096\n";

    file.close();
}

// this performs the initialize stuff
bool initialize(Config& config)
{
    std::ifstream file("config.txt");

    if (!file.is_open())
    {
        createConfigFile();
        file.open("config.txt");
    }

    string line; // for reading per line of config file

    while (file >> line) // while reading per line
    {
        if (line == "num-cpu") { file >> config.numCpu; }
        else if (line == "scheduler")
        {
            file >> config.scheduler;

            // just remove the quotes around the scheduler name
            // in the specs it has qutes
            if (!config.scheduler.empty() &&
                config.scheduler.front() == '"' && config.scheduler.back() == '"')
            {
                config.scheduler = config.scheduler.substr(1, config.scheduler.size() - 2);
            }
        }
        else if (line == "quantum-cycles") { file >> config.quantumCycles; }
        else if (line == "batch-process-freq") { file >> config.batchProcessFreq; }
        else if (line == "min-ins") { file >> config.minIns; }
        else if (line == "max-ins") { file >> config.maxIns; }
        else if (line == "delay-per-exec") { file >> config.delayPerExec; }
        else if (line == "max-overall-mem") { file >> config.maxOverallMem; }
        else if (line == "mem-per-frame") { file >> config.memPerFrame; }
        else if (line == "min-mem-per-proc") { file >> config.minMemPerProc; }
        else if (line == "max-mem-per-proc") { file >> config.maxMemPerProc; }
    }

    const long long max = 4294967296; // 2^32
    // fail checks for config value ranges
    if (config.numCpu < 1 || config.numCpu > 128) {
        cout << "Invalid num-cpu value in config.txt. Must be between 1 and 128." << endl;
        return false;
    }
    if (config.scheduler != "fcfs" && config.scheduler != "rr") {
        cout << "Invalid scheduler value in config.txt. Must be \"fcfs\" or \"rr\"." << endl;
        return false;
    }
    if (config.quantumCycles < 0 || config.quantumCycles > max) {
        cout << "Invalid quantum-cycles value in config.txt. Must be between 1 and 2^32 (4294967296)." << endl;
        return false;
    }
    if (config.batchProcessFreq < 1 || config.batchProcessFreq > max) {
        cout << "Invalid batch-process-freq value in config.txt. Must be between 1 and 2^32 (4294967296)." << endl;
        return false;
    }
    if (config.minIns < 1 || config.minIns > max) {
        cout << "Invalid min-ins value in config.txt. Must be between 1 and 2^32 (4294967296)." << endl;
        return false;
    }
    if (config.maxIns < 1 || config.maxIns > max) {
        cout << "Invalid max-ins value in config.txt. Must be between 1 and 2^32 (4294967296)." << endl;
        return false;
    }
    if (config.delayPerExec < 0 || config.delayPerExec > max) {
        cout << "Invalid delay-per-exec value in config.txt. Must be between 0 and 2^32 (4294967296)." << endl;
        return false;
    }
	if (config.maxOverallMem < 64 || config.maxOverallMem > 65536 || !(isPowerOfTwo(config.maxOverallMem))) {
		cout << "Invalid max-overall-mem value in config.txt. Must be between 2^6 (64) and 2^16 (65536) and is a power of 2." << endl;
		return false;
	}
	if (config.memPerFrame < 64 || config.memPerFrame > 65536 || !(isPowerOfTwo(config.memPerFrame))) {
		cout << "Invalid mem-per-frame value in config.txt. Must be between 2^6 (64) and 2^16 (65536) and is a power of 2." << endl;
		return false;
	}
	if (config.minMemPerProc < 64 || config.minMemPerProc > 65536 || !(isPowerOfTwo(config.minMemPerProc))) {
		cout << "Invalid min-mem-per-proc value in config.txt. Must be between 2^6 (64) and 2^16 (65536) and is a power of 2." << endl;
		return false;
	}
    if (config.maxMemPerProc < 64 || config.maxMemPerProc > 65536 || !(isPowerOfTwo(config.maxMemPerProc))) {
        cout << "Invalid max-mem-per-proc value in config.txt. Must be between 2^6 (64) and 2^16 (65536) and is a power of 2." << endl;
        return false;
    }

    file.close();

    return true;
}

int main() {
    string input;

    Config config; // config structure for config file variables
    unique_ptr<ProcessScheduler> scheduler = nullptr; // pointer to be filled later in initialize
    unique_ptr<MemoryManager> memManager = nullptr;

    unordered_map<string, function<void()>> commandMap;

    Mode screenMode = Mode::MAIN; // screen mode for main screen input
    string screen_s_process; // process name for screen -s
    string screen_r_process; // process name for screen -r
    shared_ptr<Process> activeProcessInput = nullptr; // the actual process to be occupied for screen commands

    commandMap["initialize"] = [&config, &scheduler, &memManager]() {
        if (initialize(config)) {
            // create the memory manager
            int memPerProc = getRandomPowerOfTwo(config.minMemPerProc, config.maxMemPerProc);
			memManager = make_unique<MemoryManager>(config.maxOverallMem, config.memPerFrame);

            // create the scheduler
            scheduler = make_unique<ProcessScheduler>(config.numCpu, config.scheduler, config.quantumCycles, config.delayPerExec, memManager.get());
            
            scheduler->startScheduler();

            cpuRunning = true;
            tickThread = thread([&config, &memManager](){
                while(cpuRunning){
                    tickCounter++;
                    this_thread::sleep_for(chrono::milliseconds(100));
                }
            });

            cout << "Initialized successfully" << endl;
        }
        else
        {
            cout << "Initialization failed" << endl;
        }
        };

    commandMap["scheduler-start"] = [&scheduler, &config, &memManager]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        scheduler_start(*scheduler, config, memManager.get());
        };

    commandMap["scheduler-stop"] = [&scheduler]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        if (!generating) {
            cout << "Process generation is not running." << endl;
            return;
        }
        if (generating) {
            generating = false;
            if (generatorThread.joinable()) {
                generatorThread.join();
            }
            cout << processesCreated << " processes created." << endl;
        }
        cout << "Process generation stopped." << endl;
        };

    commandMap["report-util"] = [&scheduler]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        report_util(*scheduler);
        cout << "CPU Utilization Report generated (report-util.txt)." << endl;
        };

	commandMap["process-smi"] = [&memManager, &scheduler]() {
		if (!scheduler) {
			cout << "Scheduler not initialized" << endl;
			return;
		}
        mainMenu_process_smi(memManager.get(), *scheduler);
		};

	commandMap["vmstat"] = [&memManager]() {
		if (!memManager) {
			cout << "Memory Manager not initialized" << endl;
			return;
		}
		vmstat(memManager.get());
		};

    commandMap["screen -ls"] = [&scheduler]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        screen_ls(*scheduler);
        };

    commandMap["clear"] = []() {
        system("cls");
        displayHeader();
        };

    commandMap["help"] = []() {
        cout << "Available commands:\n";
        cout << "  initialize          - Load config and start system\n";
        cout << "  scheduler-start     - Start process generation\n";
        cout << "  scheduler-stop      - Stop process generation\n";
        cout << "  report-util         - Report CPU utilization\n";
        cout << "  process-smi         - Summarized view of memory usage and a list of processes\n";
        cout << "  vmstat              - Detailed view of the processes, memory, pages\n";
        cout << "  screen -ls          - List running and finished processes\n";
        cout << "  screen -s <name>    - Start a new process with the given name\n";
        cout << "  screen -c <name> <mem> \"<instrs>\" - Create a process with custom instructions\n";
        cout << "  screen -r <name>    - Review an existing process with the given name\n";
        cout << "  clear               - Clear the console screen\n";
        cout << "  help                - Show this help message\n";
        cout << "  exit                - Exit the program\n";
        };

    bool running = true;
    commandMap["exit"] = [&running, &scheduler, &memManager]() {
        cout << "Exiting program." << endl;
        if (generating) {
            generating = false;
            if (generatorThread.joinable()) {
                generatorThread.join();
            }
        }
        
        cpuRunning = false;
        if (tickThread.joinable()){
            tickThread.join();
        }
        cout << "ticks: " << tickCounter;
        cout << "pages loaded: " << memManager->getPagedIn();
        cout << "pages evicted: " << memManager->getPagedOut();

        if (scheduler) {
            scheduler->stopScheduler();
        }
        running = false;
        };

    displayHeader();

    while (running) {
        cout << "Enter a command: ";
        getline(cin, input);

        if (screenMode == Mode::MAIN) { // MAIN mode for MAIN inputs

            //
            // screen -c <process name> <memory size> "<instructions>" command
            //
            if (input.find("screen -c ") == 0)
            {
                if (!scheduler) {
                    cout << "Scheduler not initialized" << endl;
                    continue;
                }

                std::string rest = input.substr(string("screen -c ").size());

                size_t firstQuote = rest.find('"');
                size_t lastQuote = rest.rfind('"');
                if (firstQuote == std::string::npos || lastQuote == firstQuote)
                {
                    cout << "Usage: screen -c <process name> <memory size> \"<instructions>\"" << endl;
                    continue;
                }

                std::string header = rest.substr(0, firstQuote);
                std::string instructionText = rest.substr(firstQuote + 1, lastQuote - firstQuote - 1);

                std::istringstream headerStream(header);
                std::string procName;
                long long memSize = 0;
                headerStream >> procName >> memSize;

                if (procName.empty() || headerStream.fail())
                {
                    cout << "Usage: screen -c <process name> <memory size> \"<instructions>\"" << endl;
                    continue;
                }

                if (memSize < 64 || memSize > 65536 || !isPowerOfTwo(static_cast<int>(memSize)))
                {
                    cout << "Invalid memory size. Must be a power of 2 between 64 and 65536." << endl;
                    continue;
                }

                try
                {
                    auto p = make_shared<Process>(pidCounter++, "screen", procName, memSize, config.memPerFrame);
                    auto cmds = CommandParser::parse(instructionText, memSize, memManager.get());
                    for (auto& c : cmds) p->addCommand(c);

                    {
                        lock_guard<mutex> lock(processListMutex);
                        processList.push_back(p);
                    }
                    scheduler->addProcess(p, rand() % scheduler->getnumCores());

                    activeProcessInput = p;
                    screenMode = Mode::SUBSCREEN;
                    system("cls");
                }
                catch (const std::exception& e)
                {
                    cout << "Failed to create process: " << e.what() << endl;
                }

                continue;
            }

            //
            // screen -s <process name> command
            //
            if (input.find("screen -s ") == 0)
            {
                screen_s_process = input.substr(string("screen -s ").size()); // process name "screen -s <process name>"

                if (!scheduler) {
                    cout << "Scheduler not initialized" << endl;
                    continue;
                }

                if (screen_s_process.empty())
                {
                    cout << "Usage: screen -s <process name>" << endl;
                    continue;
                }

                /* new process creation */
                int numCommands = getRandomInt(config.minIns, config.maxIns);
                int memPerProc = getRandomPowerOfTwo(config.minMemPerProc, config.maxMemPerProc);

                shared_ptr<Process> newProcess = makeProcess(pidCounter++, screen_s_process, numCommands, memPerProc, config.memPerFrame, memManager.get());
                {   // lock when adding a new process
                    lock_guard<mutex> lock(processListMutex);
                    processList.push_back(newProcess);
                }
                scheduler->addProcess(newProcess, rand() % scheduler->getnumCores()); // assign to a random core

                activeProcessInput = newProcess;
                screenMode = Mode::SUBSCREEN;
                system("cls");
                continue;
            }

            // 
            // screen -r <process name> command
            //
            if (input.find("screen -r ") == 0)
            {
                screen_r_process = input.substr(string("screen -r ").size()); // process name "screen -r <process name>"

                if (!scheduler) {
                    cout << "Scheduler not initialized" << endl;
                    continue;
                }

                if (screen_r_process.empty())
                {
                    cout << "Usage: screen -r <process name>" << endl;
                    continue;
                }

                /* keeper of found process in the list to pass onto the higher level activeProcessInput
                 * for screen -r */
                shared_ptr<Process> foundProcess = nullptr;

                lock_guard<mutex> lock(processListMutex);
                // look for target process in the list
                for (shared_ptr<Process>& p : processList)
                {
                    if (p->getName() == screen_r_process)
                    {
                        foundProcess = p;
                        break;
                    }
                }

                // if process name not found
                if (!foundProcess)
                {
                    cout << "Process " << screen_r_process << " not found." << endl;
                    continue;
                }

                // if process name not finished execution
                if (foundProcess->getState() == Process::TERMINATED)
                {
                    cout << "Process " << screen_r_process << " not found." << endl;
                    continue;
                }

                activeProcessInput = foundProcess;
                screenMode = Mode::SUBSCREEN;
                system("cls");
                continue;
            }

            //
            // minor input validation area
            //
            if (input.find("screen -c") == 0)
            {
                cout << "Usage: screen -c <process name> <memory size> \"<instructions>\"" << endl;
                continue;
            }

            if (input.find("screen -s") == 0)
            {
                cout << "Usage: screen -s <process name>" << endl;
                continue;
            }

            if (input.find("screen -r") == 0)
            {
                cout << "Usage: screen -r <process name>" << endl;
                continue;
            }

            //
            // commandMap
            //
            auto it = commandMap.find(input);
            if (it != commandMap.end()) {
                it->second();
            }
            else {
                cout << "Unknown command: " << input << endl;
            }
        }
        else if (screenMode == Mode::SUBSCREEN) // SUBSCREEN mode for SUBSCREEN inputs
        {
            if (input == "process-smi")
            {
                process_smi(*scheduler, activeProcessInput);
            }

            else if (input == "exit") {
                screenMode = Mode::MAIN;
                system("cls");
                activeProcessInput = nullptr;
                displayHeader();
            }
            else {
                cout << "Unknown command: " << input << endl;
            }
        }
    }

    return 0;
}