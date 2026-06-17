#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cstdlib>
#include <iomanip>

#include "Process.h"
#include "PrintCommand.h"

using namespace std;

// ---------------------------------------------------------------------------
// Global process list (the scheduler will manage this later)
// ---------------------------------------------------------------------------
vector<shared_ptr<Process>> processList;

// ---------------------------------------------------------------------------
// Header
// ---------------------------------------------------------------------------
void displayHeader() {
    cout << " _____ _____ _____ _____ _____ _____ __ __ \n";
    cout << "|     |   __|     |  _  |   __|   __|  |  |\n";
    cout << "|   --|__   |  |  |   __|   __|__   |_   _|\n";
    cout << "|_____|_____|_____|__|  |_____|_____| |_|  \n";

    cout << "Hello, Welcome to CSOPESY commandline!" << endl;
    cout << "Type 'exit' to quit, 'clear' to clear the screen.\n" << endl;
    cout << "** IMPORTANT: Type 'initialize' to load config and start system **" << endl;
}

// ---------------------------------------------------------------------------
// screen -ls display
// Shows running processes and finished processes matching the reference UI.
// ---------------------------------------------------------------------------
void screen_ls() {
    // TODO: your scheduler teammate will fill in real CPU utilization numbers
    cout << "CPU Utilization: " << endl;
    cout << "Cores used: "      << endl;
    cout << "Cores available: " << endl << endl;

    cout << "---------------------------------------\n";
    cout << "Running processes:\n";
    for (auto& p : processList) {
        if (p->getState() == Process::RUNNING) {
            cout << left  << setw(12) << p->getName()
                 << " "   << p->getCreationTime()
                 << "   Core: " << p->getCpuCoreID()
                 << "   " << p->getCommandCounter()
                 << " / " << p->getTotalCommands()
                 << "\n";
        }
    }

    cout << "\nFinished processes:\n";
    for (auto& p : processList) {
        if (p->getState() == Process::TERMINATED) {
            cout << left  << setw(12) << p->getName()
                 << " "   << p->getCreationTime()
                 << "   Finished"
                 << "   " << p->getTotalCommands()
                 << " / " << p->getTotalCommands()
                 << "\n";
        }
    }
    cout << "---------------------------------------\n";
}

// ---------------------------------------------------------------------------
// Helper: build one process with numCommands PrintCommands attached
// ---------------------------------------------------------------------------
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
void initialize() {
    if (!processList.empty()) {
        cout << "[initialize] System already initialized.\n";
        return;
    }

    const int NUM_PROCESSES   = 10;
    const int CMDS_PER_PROCESS = 100;

    for (int i = 1; i <= NUM_PROCESSES; i++) {
        // zero-pad name: process01, process02, ...
        string name = "process" + (i < 10 ? "0" : "") + to_string(i);
        processList.push_back(makeProcess(i, name, CMDS_PER_PROCESS));
    }

    cout << "[initialize] Created " << NUM_PROCESSES
         << " processes with " << CMDS_PER_PROCESS
         << " print commands each.\n";
    cout << "[initialize] TODO: hand processList to your FCFS scheduler here.\n";

    // -----------------------------------------------------------------------
    // STARTER DEMO (remove once the real scheduler is wired in):
    // Run all processes sequentially on a single fake core so you can verify
    // the .txt files are produced correctly before the scheduler is ready.
    // -----------------------------------------------------------------------
    cout << "[demo] Running all processes on core 0 (single-threaded demo)...\n";
    for (auto& p : processList) {
        p->setCpuCoreID(0);
        p->openLogFile();                          // creates e.g. process01.txt
        p->setProcessState(Process::RUNNING);

        while (!p->isFinished()) {
            p->executeNextCommand();               // PrintCommand -> Process::log() -> file
        }

        p->setProcessState(Process::TERMINATED);
        cout << "  [done] " << p->getName()
             << "  " << p->getCommandCounter()
             << " / " << p->getTotalCommands() << "\n";
    }
    cout << "[demo] All processes finished. Check the .txt files in your working directory.\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    string input;

    unordered_map<string, function<void()>> commandMap;

    commandMap["initialize"] = []() {
        initialize();
    };

    commandMap["screen"] = []() {
        cout << "screen command recognized. Doing something." << endl;
    };

    commandMap["scheduler-start"] = []() {
        // TODO: your scheduler teammate starts the FCFS scheduler thread here
        cout << "scheduler-start command recognized. Doing something." << endl;
    };

    commandMap["scheduler-stop"] = []() {
        // TODO: your scheduler teammate stops the scheduler thread here
        cout << "scheduler-stop command recognized. Doing something." << endl;
    };

    commandMap["report-util"] = []() {
        cout << "report-util command recognized. Doing something." << endl;
    };

    commandMap["screen -ls"] = []() {
        screen_ls();
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

        auto it = commandMap.find(input);
        if (it != commandMap.end()) {
            it->second();
        } else {
            cout << "Unknown command: " << input << endl;
        }
    }

    return 0;
}