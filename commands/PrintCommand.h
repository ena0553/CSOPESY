#pragma once

#include "ICommand.h"
#include <string>

class PrintCommand : public ICommand
{
public:
    PrintCommand(const std::string& message);

    CommandType getType() const override;

    void execute(Process& process) override;

    std::string toString() const override;

private:
    std::string message;
};