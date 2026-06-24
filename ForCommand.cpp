#include "ForCommand.h"

ForCommand::ForCommand(int count, std::shared_ptr<ICommand> instructions)
    : count(count), instructions(instructions)
{
}

ICommand::CommandType ForCommand::getType() const {
    return CommandType::COMPUTE;
}

void ForCommand::execute(Process& process) {
    for (int i = 0; i < count; i++) {
        instructions->execute(process);
    }
}

std::string ForCommand::toString() const {
    return "FOR: " + std::to_string(count) + " iterations";
}