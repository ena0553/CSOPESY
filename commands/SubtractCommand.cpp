#include "SubtractCommand.h"
#include "../Process.h"

SubtractCommand::SubtractCommand(int result, int var1, int var2)
    : result(result), var1(var1), var2(var2)
{
}

ICommand::CommandType SubtractCommand::getType() const {
    return CommandType::COMPUTE;
}

void SubtractCommand::execute(Process& process) {
    int result = var1 - var2;

    process.log(
        std::to_string(var1) + " - " +
        std::to_string(var2) + " = " +
        std::to_string(result)
    );
}

std::string SubtractCommand::toString() const {
    return "SUBTRACT: " + std::to_string(var1) + " - " + std::to_string(var2) + " = " + std::to_string(result);
}