#include "PrintCommand.h"
#include "Process.h"

PrintCommand::PrintCommand(const std::string& message)
    : message(message)
{
}

ICommand::CommandType PrintCommand::getType() const {
    return CommandType::IO;
}

void PrintCommand::execute(Process& process) {
    process.log(message);
}

std::string PrintCommand::toString() const {
    return message;
}