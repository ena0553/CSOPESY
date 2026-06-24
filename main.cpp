#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <iomanip>
#include <random>

#include "FCFSScheduler.h"
#include "Process.h"
#include "PrintCommand.h"

#include <thread>
#include <atomic>
#include <mutex>

#include <fstream>

using namespace std;

// Global process list (the scheduler will manage this later)
vector<shared_ptr<Process>> processList;

static int pidCounter = 1; // global PID counter unique PID assignment for processes

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
};

enum class Mode {
    MAIN,
    SUBSCREEN
};

// Header
void displayHeader() {
    cout << " _____ _____ _____ _____ _____ _____ __ __ \n";
    cout << "|     |   __|     |  _  |   __|   __|  |  |\n";
    cout << "|   --|__   |  |  |   __|   __|__   |_   _|\n";
    cout << "|_____|_____|_____|__|  |_____|_____| |_|  \n";

    cout << "Hello, Welcome to CSOPESY commandline!" << endl;
    cout << "Type 'exit' to quit, 'clear' to clear the screen.\n" << endl;
    cout << "** IMPORTANT: Type 'initialize' to load config and start system **" << endl;
}

// screen -ls display
void screen_ls(FCFSScheduler& scheduler) {

    int used = scheduler.getBusyCores();
    int totalCores = scheduler.getnumCores();

    cout << "CPU Utilization: " << (used * 100.0 / totalCores) << endl;
    cout << "Cores used: " << used << endl;
    cout << "Cores available: " << (totalCores - used) << endl;

    cout << "---------------------------------------\n";
    cout << "Running processes:\n";
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

    cout << "\nFinished processes:\n";
    for (auto& p : processList) {
        if (p->getState() == Process::TERMINATED) {
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

// process-smi command for screen -s
void process_smi(FCFSScheduler& scheduler, shared_ptr<Process>& process)
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

// Helper: get a random integer between minimum instructions and maximum instructions
int getRandomInt(int minIns, int maxIns)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(minIns, maxIns);
    return dist(gen);
}

// Helper: build one process with numCommands PrintCommands attached
// TODO: change this to use the random instructions, not just print commands
shared_ptr<Process> makeProcess(int pid, const string& name, int numCommands) {
    auto p = make_shared<Process>(pid, "screen", name, "0");
    for (int i = 0; i < numCommands; i++) {
        string msg = "Hello world from " + name + "!";
        p->addCommand(make_shared<PrintCommand>(msg));
    }
    return p;
}

// ---------------------------------------------------------------------------
// "initialize" — creates 10 processes with 100 print commands each
// (HW test case: 10 processes, 100 commands each)
// The scheduler will pick these up from processList and run them.
// ---------------------------------------------------------------------------
void scheduler_start(FCFSScheduler& scheduler, Config config) {
    // TODO: change this part for cpu ticks thing in specs
    const int NUM_PROCESSES = 10;
    int numCommands = 100; 

    for (int i = 1; i <= NUM_PROCESSES; i++) {
        string name = string("process") + (i < 10 ? "0" : "") + to_string(i);
        numCommands = getRandomInt(config.minIns, config.maxIns);
        processList.push_back(makeProcess(pidCounter, name, numCommands));
        pidCounter++;
    }

    cout << "Created " << NUM_PROCESSES << " processes." << endl;

    //no longer randomized, just queues from 0 to 3 since random doesn't equally distribute it
    int coreID = 0;

    for (auto& process : processList) {
        scheduler.addProcess(process, coreID);
        coreID = (coreID + 1) % scheduler.getnumCores();
    }
    scheduler.startScheduler();

}

// this creates the config file if it's not in the folder yet
void createConfigFile()
{
    std::ofstream file("config.txt");

    file << "num-cpu 4\n";
    file << "scheduler \"fcfs\"\n";
    file << "quantum-cycles 5\n";
    file << "batch-process-freq 1\n";
    file << "min-ins 1000\n";
    file << "max-ins 2000\n";
    file << "delay-per-exec 0\n";

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
    if (config.quantumCycles < 1 || config.quantumCycles > max) {
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

    file.close();

    return true;
}

int main() {
    string input;

    Config config; // config structure for config file variables
    unique_ptr<FCFSScheduler> scheduler = nullptr; // pointer to be filled later in initialize

    unordered_map<string, function<void()>> commandMap;

    Mode screenMode = Mode::MAIN; // screen mode for main screen input
    string screen_s_process; // process name for screen -s
    string screen_r_process; // process name for screen -r
    shared_ptr<Process> activeProcessInput = nullptr; // the actual process to be occupied for screen commands

    commandMap["initialize"] = [&config, &scheduler]() {
        if (initialize(config)) {
            scheduler = make_unique<FCFSScheduler>(config.numCpu);
            cout << "Initialized successfully" << endl;
        }
        else
        {
            cout << "Initialization failed" << endl;
        }
        };

    commandMap["scheduler-start"] = [&scheduler, &config]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        scheduler_start(*scheduler, config);
        };

    commandMap["scheduler-stop"] = [&scheduler]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        scheduler->stopScheduler();
        cout << "Scheduler stopped." << endl;
        };

    commandMap["report-util"] = []() {
        cout << "report-util command recognized. Doing something." << endl;
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

    bool running = true;
    commandMap["exit"] = [&running]() {
        cout << "Exiting program." << endl;
        running = false;
        };

    displayHeader();

    while (running) {
        cout << "Enter a command: ";
        getline(cin, input);

        if (screenMode == Mode::MAIN) { // MAIN mode for MAIN inputs

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

                shared_ptr<Process> newProcess = makeProcess(pidCounter++, screen_s_process, numCommands);
                processList.push_back(newProcess);

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