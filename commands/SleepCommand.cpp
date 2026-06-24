#include "SleepCommand.h"
#include "../Process.h"


SleepCommand::SleepCommand(uint8_t ticks)
    : ticks(ticks)
{
}

ICommand::CommandType SleepCommand::getType() const {
    return CommandType::IO;
}

void SleepCommand::execute(Process& process) {
    process.setSleepTicks(ticks);
    process.setProcessState(Process::WAITING);
    process.log("Sleeping for " + std::to_string(ticks) + " ticks");
}

std::string SleepCommand::toString() const {
    return "SLEEP: " + std::to_string(ticks) + " ticks";
}