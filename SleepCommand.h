#pragma once

#include "ICommand.h"
#include <string>

class SleepCommand : public ICommand
{
public:
    SleepCommand(int duration);

    CommandType getType() const override;
    void execute(Process& process) override;
    std::string toString() const override;

private:
    int duration;
};
