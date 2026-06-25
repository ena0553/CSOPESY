#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <iomanip>
#include <random>

#include "ProcessScheduler.h"
#include "Process.h"
#include "commands/PrintCommand.h"
#include "commands/SleepCommand.h"
#include "commands/SubtractCommand.h"
#include "commands/ForCommand.h"
#include "commands/DeclareCommand.h"
#include "commands/AddCommand.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <thread>

#include <fstream>

using namespace std;

// Global process list (the scheduler will manage this later)
mutex processListMutex;
vector<shared_ptr<Process>> processList;
atomic<int> pidCounter = 1; // global PID counter unique PID assignment for processes

// global generator thread for generating process in scheduler-start
atomic<bool> generating = false;
thread generatorThread;

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

void report_util(ProcessScheduler& scheduler) {
    ofstream outFile("report-util.txt");

    int used = scheduler.getBusyCores();
    int totalCores = scheduler.getnumCores();

    outFile << "CPU Utilization: " << (used * 100.0 / totalCores) << endl;
    outFile << "Cores used: " << used << endl;
    outFile << "Cores available: " << (totalCores - used) << endl;

    outFile << "---------------------------------------\n";
    outFile << "Running processes:\n";
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

    outFile << "\nFinished processes:\n";
    for (auto& p : processList) {
        if (p->getState() == Process::TERMINATED) {
            outFile << left << setw(12) << p->getName()
                << " " << p->getCreationTime()
                << "   Finished"
                << "   " << p->getTotalCommands()
                << " / " << p->getTotalCommands()
                << "\n";
        }
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
shared_ptr<Process> makePrintProcess(int pid, const string& name, int numCommands) {
    auto p = make_shared<Process>(pid, "screen", name, "0");
    for (int i = 0; i < numCommands; i++) {
        string msg = "Hello world from " + name + "!";
        p->addCommand(make_shared<PrintCommand>(msg));
    }
    return p;
}

shared_ptr<Process> makeProcess (int pid, const string& name, int numCommands) {
    auto p = make_shared<Process>(pid, "screen", name, "0");
    for (int i = 0; i < numCommands; i++) {
        // Randomly choose a command type
        int commandType = getRandomInt(1, 6); // 1: Print, 2: Sleep, 3: Subtract, 4: For, 5: Declare, 6: Add

        if (commandType == 1) {
            string msg = "Hello world from " + name + "!";
            p->addCommand(make_shared<PrintCommand>(msg));
        }
        else if (commandType == 2) {
            uint8_t ticks = getRandomInt(1, 10); // Random sleep ticks between 1 and 10
            p->addCommand(make_shared<SleepCommand>(ticks));
        }
        else if (commandType == 3) {
            int var1 = getRandomInt(1, 100);
            int var2 = getRandomInt(1, 100);
            int dest = getRandomInt(1, 100); // Destination variable for the result
            p->addCommand(make_shared<SubtractCommand>(dest, var1, var2));
        } else if (commandType == 4) {
            int count = getRandomInt(1, 5); // Number of iterations for ForCommand
            std::vector<std::shared_ptr<ICommand>> instructions;
            // Add a few random commands inside the ForCommand
            for (int j = 0; j < getRandomInt(1, 3); j++) {
                int innerCommandType = getRandomInt(1, 3); // 1: PrintCommand, 2: SleepCommand, 3: SubtractCommand
                if (innerCommandType == 1) {
                    string msg = "Hello from inside FOR loop!";
                    instructions.push_back(make_shared<PrintCommand>(msg));
                }
                else if (innerCommandType == 2) {
                    uint8_t ticks = getRandomInt(1, 5);
                    instructions.push_back(make_shared<SleepCommand>(ticks));
                }
                else if (innerCommandType == 3) {
                    int var1 = getRandomInt(1, 50);
                    int var2 = getRandomInt(1, 50);
                    int dest = getRandomInt(1, 50);
                    instructions.push_back(make_shared<SubtractCommand>(dest, var1, var2));
                }
            }
            p->addCommand(make_shared<ForCommand>(count, instructions));
            
        } else if (commandType == 5) {
            // DECLARE: random variable name, random initial value
            std::string varName = "var" + std::to_string(getRandomInt(1, 10));
            uint16_t initVal = static_cast<uint16_t>(getRandomInt(0, 1000));
            p->addCommand(make_shared<DeclareCommand>(varName, initVal));
        } else if (commandType == 6) {
            // ADD: dest = operand1 + operand2 (mix of variables and literals)
            std::string dest  = "var" + std::to_string(getRandomInt(1, 10));
            std::string src1  = "var" + std::to_string(getRandomInt(1, 10));
            uint16_t    lit   = static_cast<uint16_t>(getRandomInt(0, 500));
            // operand1 is a variable, operand2 is a literal
            p->addCommand(make_shared<AddCommand>(dest, Operand{src1}, Operand{lit}));
        }
    }
    return p;
}



void scheduler_start(ProcessScheduler& scheduler, Config config) {
    if (generating) {
        cout << "Process generation is already running." << endl;
        return;
    }
    cout << "Generating processes..." << endl;
    generating = true;

    generatorThread = std::thread([&scheduler, config]() {
        int coreID = 0;
        while (generating) {
            int numCommands = getRandomInt(config.minIns, config.maxIns);
            string name = string("process") + (pidCounter < 10 ? "0" : "") + to_string(pidCounter);
            auto newProcess = makeProcess(pidCounter++, name, numCommands);
            {
                lock_guard<mutex> lock(processListMutex);
                processList.push_back(newProcess);
            }
            

            // Assign the process to a core in a round-robin fashion
            scheduler.addProcess(newProcess, coreID);
            coreID = (coreID + 1) % scheduler.getnumCores();

            std::this_thread::sleep_for(std::chrono::seconds(config.batchProcessFreq));
        }
    });
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
    unique_ptr<ProcessScheduler> scheduler = nullptr; // pointer to be filled later in initialize

    unordered_map<string, function<void()>> commandMap;

    Mode screenMode = Mode::MAIN; // screen mode for main screen input
    string screen_s_process; // process name for screen -s
    string screen_r_process; // process name for screen -r
    shared_ptr<Process> activeProcessInput = nullptr; // the actual process to be occupied for screen commands

    commandMap["initialize"] = [&config, &scheduler]() {
        if (initialize(config)) {
            scheduler = make_unique<ProcessScheduler>(config.numCpu, config.scheduler, config.quantumCycles);
            scheduler->startScheduler();
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
        if (!generating) {
            cout << "Process generation is not running." << endl;
            return;
        }
        if (generating) {
            generating = false;
            if (generatorThread.joinable()) {
                generatorThread.join();
            }
        }
        cout << "Process generation stopped." << endl;
        };

    commandMap["report-util"] = [&scheduler]() {
        if (!scheduler) {
            cout << "Scheduler not initialized" << endl;
            return;
        }
        report_util(*scheduler);
        cout << "CPU Utilization Report generated (report-util.txt).";
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
        cout << "  screen -ls          - List running and finished processes\n";
        cout << "  screen -s <name>    - Start a new process with the given name\n";
        cout << "  screen -r <name>    - Review an existing process with the given name\n";
        cout << "  clear               - Clear the console screen\n";
        cout << "  help                - Show this help message\n";
        cout << "  exit                - Exit the program\n";
        };

    bool running = true;
    commandMap["exit"] = [&running, &scheduler]() {
        cout << "Exiting program." << endl;
        if (generating) {
            generating = false;
            if (generatorThread.joinable()) {
                generatorThread.join();
            }
        }
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