#pragma once

#include "ICommand.h"
#include <string>
#include <cstdint>

class SleepCommand : public ICommand
{
public:
    SleepCommand(uint8_t ticks);

    CommandType getType() const override;
    void execute(Process& process) override;
    std::string toString() const override;

private:
    uint8_t ticks; // number of ticks to sleep
};
