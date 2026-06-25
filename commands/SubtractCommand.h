#pragma once

#include "ICommand.h"
#include <string>
#include <variant>
#include <cstdint>

using Operand = std::variant<std::string, uint16_t>;

class SubtractCommand : public ICommand
{
public:
    // dest     — variable name to store the result
    // operand1 — variable name OR literal uint16
    // operand2 — variable name OR literal uint16
    SubtractCommand(const std::string& dest, Operand operand1, Operand operand2);

    CommandType getType() const override;
    void execute(Process& process) override;
    std::string toString() const override;

private:
    std::string dest;
    Operand operand1;
    Operand operand2;

    // Resolves an Operand to its uint16 value, auto-declaring if it is a variable.
    uint16_t resolve(Operand& operand, Process& process) const;

    // Returns a display string for an operand (for logging).
    std::string operandStr(const Operand& operand) const;
};