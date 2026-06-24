#include "SleepCommand.h"
#include "Process.h"

#include <thread>
#include <chrono>

SleepCommand::SleepCommand(int duration)
    : duration(duration)
{
}

ICommand::CommandType SleepCommand::getType() const {
    return CommandType::IO;
}

void SleepCommand::execute(Process&) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(duration)
    );
}

std::string SleepCommand::toString() const {
    return "SLEEP: " + std::to_string(duration) + " ms";
}