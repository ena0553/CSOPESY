#pragma once

#include "ICommand.h"
#include <string>

class SubtractCommand : public ICommand
{
public:
    SubtractCommand(int result, int var1, int var2);

    CommandType getType() const override;
    void execute(Process& process) override;
    std::string toString() const override;

private:
    int result;
    int var1;
    int var2;
};