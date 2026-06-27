#include "ForCommand.h"
#include "../Process.h"

ForCommand::ForCommand(int count, std::vector<std::shared_ptr<ICommand>> instructions)
    : count(count), instructions(instructions)
{
}

ICommand::CommandType ForCommand::getType() const {
    return CommandType::CONTROL;
}

void ForCommand::execute(Process& process) {
    process.incrementCommandCounter();
    process.log("Entering FOR loop (" + std::to_string(count) + " iterations)");
    for (int i = 0; i < count; i++) {
        process.log("Iteration " + std::to_string(i + 1) + "/" + std::to_string(count));
        for (const auto& instruction : instructions) {
            instruction->execute(process);
        }
    }
    process.log("Exiting FOR loop");
}

std::string ForCommand::toString() const {
    return "FOR: " + std::to_string(count) + " iterations";
}

int ForCommand::countCommands() const {
    int total = 1; // Counting the FOR command itself

    for (const auto& cmd : instructions) {
        total += count * cmd->countCommands();
    }

    return total;
}