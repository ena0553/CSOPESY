#pragma once

#include "ICommand.h"
#include <string>
#include <cstdint>

class DeclareCommand : public ICommand
{
public:
    // varName  — the variable to declare
    // value    — the initial uint16 value (default 0)
    DeclareCommand(const std::string& varName, uint16_t value = 0);

    CommandType getType() const override;
    void execute(Process& process) override;
    std::string toString() const override;

private:
    std::string varName;
    uint16_t    value;
};