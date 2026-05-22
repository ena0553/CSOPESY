#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdlib>

using namespace std;

void displayHeader() {                                          
    cout << " _____ _____ _____ _____ _____ _____ __ __ \n";
    cout << "|     |   __|     |  _  |   __|   __|  |  |\n";
    cout << "|   --|__   |  |  |   __|   __|__   |_   _|\n";
    cout << "|_____|_____|_____|__|  |_____|_____| |_|  \n";

    cout << "Hello, Welcome to CSOPESY commandline!" << endl;
    cout << "Type 'exit' to quit, 'clear' to clear the screen.\n" << endl;
    cout << "** IMPORTANT: Type 'initialize' to load config and start system **" << endl;
}

int main() {
    string input;

    // command -> action
    unordered_map<string, function<void()>> commandMap;

    commandMap["initialize"] = []() {
        cout << "initialize command recognized. Doing something." << endl;
    };

    commandMap["screen"] = []() {
        cout << "screen command recognized. Doing something." << endl;
    };

    commandMap["scheduler-start"] = []() {
        cout << "scheduler-start command recognized. Doing something." << endl;
    };

    commandMap["scheduler-stop"] = []() {
        cout << "scheduler-stop command recognized. Doing something." << endl;
    };

    commandMap["report-util"] = []() {
        cout << "report-util command recognized. Doing something." << endl;
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

        auto commandIt = commandMap.find(input);
        if (commandIt != commandMap.end()) {
            commandIt->second();
        } else {
            cout << "Unknown command: " << input << endl;
        }
    }

    return 0;
}