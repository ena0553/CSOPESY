#include "PrintCommand.h"
#include "../Process.h"
#include <thread>
#include <chrono>

PrintCommand::PrintCommand(const std::string& message)
    : message(message)
{
}

ICommand::CommandType PrintCommand::getType() const {
    return CommandType::IO;
}

void PrintCommand::execute(Process& process) {
    process.log(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

std::string PrintCommand::toString() const {
    return message;
}