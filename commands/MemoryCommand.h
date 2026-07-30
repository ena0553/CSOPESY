#pragma once

#include "ICommand.h"
#include <string>
#include <cstdint>
#include <variant>

using Operand = std::variant<std::string, uint16_t>;

class MemoryManager;

class MemoryCommand : public ICommand
{
public:

    enum class Operation
    {
        READ,
        WRITE
    };

    MemoryCommand(Operation operation,
                  MemoryManager* memoryManager,
                  uint32_t address,
                  const std::string& variable,
                  Operand source);

    CommandType getType() const override;

    void execute(Process& process) override;

    std::string toString() const override;

private:

    Operation operation;

    MemoryManager* memoryManager;

    uint32_t address;

    // READ destination
    std::string variable;

    // WRITE source
    Operand source; // can be a variable name or a literal value

    uint16_t resolve(const Operand& operand, Process& process) const;

    std::string operandStr(const Operand& operand) const;

};