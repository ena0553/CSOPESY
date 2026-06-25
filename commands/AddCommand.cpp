#include "AddCommand.h"
#include "../Process.h"

AddCommand::AddCommand(const std::string& dest, Operand operand1, Operand operand2)
    : dest(dest), operand1(operand1), operand2(operand2)
{
}

ICommand::CommandType AddCommand::getType() const
{
    return CommandType::COMPUTE;
}

void AddCommand::execute(Process& process)
{
    uint16_t val1 = resolve(operand1, process);
    uint16_t val2 = resolve(operand2, process);

    // Use int32 for addition before clamping to catch overflow correctly.
    int32_t result = static_cast<int32_t>(val1) + static_cast<int32_t>(val2);

    // set() handles clamping to [0, 65535] and auto-declares dest.
    process.getSymbolTable().set(dest, result);

    process.log(
        dest + " = " + operandStr(operand1) + "(" + std::to_string(val1) + ")"
        + " + " + operandStr(operand2) + "(" + std::to_string(val2) + ")"
        + " = " + std::to_string(process.getSymbolTable().get(dest))
    );
}

std::string AddCommand::toString() const
{
    return "ADD " + dest + " = " + operandStr(operand1) + " + " + operandStr(operand2);
}

uint16_t AddCommand::resolve(Operand& operand, Process& process) const
{
    if (std::holds_alternative<std::string>(operand))
    {
        // Variable — auto-declared at 0 by get() if not yet declared.
        return process.getSymbolTable().get(std::get<std::string>(operand));
    }
    else
    {
        return std::get<uint16_t>(operand);
    }
}

std::string AddCommand::operandStr(const Operand& operand) const
{
    if (std::holds_alternative<std::string>(operand))
        return std::get<std::string>(operand);
    else
        return std::to_string(std::get<uint16_t>(operand));
}