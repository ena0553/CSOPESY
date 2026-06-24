#include "ForCommand.h"
#include "Process.h"

ForCommand::ForCommand(int count, std::shared_ptr<ICommand> instructions)
    : count(count), instructions(instructions)
{
}

ICommand::CommandType ForCommand::getType() const {
    return CommandType::COMPUTE;
}

void ForCommand::execute(Process& process) {
    process.log("Entering FOR loop (" + std::to_string(count) + " iterations)");
    for (int i = 0; i < count; i++) {
        process.log("Iteration " + std::to_string(i + 1) + "/" + std::to_string(count));
        instructions->execute(process);
    }
    process.log("Exiting FOR loop");
}

std::string ForCommand::toString() const {
    return "FOR: " + std::to_string(count) + " iterations";
}