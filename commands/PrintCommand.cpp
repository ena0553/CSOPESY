#include "PrintCommand.h"
#include "../Process.h"
#include <thread>
#include <chrono>
#include <cstdint>

PrintCommand::PrintCommand(const std::string& message)
    : message(message)
{
}

PrintCommand::PrintCommand(const std::string& message, const std::string& varName)
    : message(message), varName(varName)
{
}

ICommand::CommandType PrintCommand::getType() const {
    return CommandType::IO;
}

void PrintCommand::execute(Process& process) {
    if (varName.has_value()) {
        uint16_t val = process.getSymbolTable().get(varName.value());
        process.log(message + std::to_string(val));
    } else {
        process.log(message);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

std::string PrintCommand::toString() const {
    return message;
}