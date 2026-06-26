#include "SleepCommand.h"
#include "../Process.h"

extern std::atomic<long long> tickCounter;


SleepCommand::SleepCommand(uint8_t ticks)
    : ticks(ticks)
{
}

ICommand::CommandType SleepCommand::getType() const {
    return CommandType::IO;
}

void SleepCommand::execute(Process& process) {
    process.incrementCommandCounter();
    process.setWakeTick(tickCounter.load() + ticks);
    process.setProcessState(Process::WAITING);
    process.log("Sleeping for " + std::to_string(ticks) + " ticks (wake at tick " + 
                    std::to_string(process.getWakeTick()) + ")");
}

std::string SleepCommand::toString() const {
    return "SLEEP: " + std::to_string(ticks) + " ticks";
}