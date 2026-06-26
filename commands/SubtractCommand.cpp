#include "SubtractCommand.h"
#include "../Process.h"

SubtractCommand::SubtractCommand(const std::string& dest, Operand operand1, Operand operand2)
    : dest(dest), operand1(operand1), operand2(operand2)
{
}

ICommand::CommandType SubtractCommand::getType() const {
    return CommandType::COMPUTE;
}

void SubtractCommand::execute(Process& process) {
    process.incrementCommandCounter();
 
    uint16_t val1 = resolve(operand1, process);
    uint16_t val2 = resolve(operand2, process);
    // Use int32 for subtraction before clamping to catch overflow correctly.
    int32_t result = static_cast<int32_t>(val1) - static_cast<int32_t>(val2);

    // set() handles clamping to [0, 65535] and auto-declares dest.
    process.getSymbolTable().set(dest, result);

    process.log(
        dest + " = " + operandStr(operand1) + "(" + std::to_string(val1) + ")"
        + " - " + operandStr(operand2) + "(" + std::to_string(val2) + ")"
        + " = " + std::to_string(process.getSymbolTable().get(dest))
    );
}

std::string SubtractCommand::toString() const {
    return "SUBTRACT: " + dest + " = " + operandStr(operand1) + " - " + operandStr(operand2);
}

uint16_t SubtractCommand::resolve(Operand& operand, Process& process) const
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

std::string SubtractCommand::operandStr(const Operand& operand) const
{
    if (std::holds_alternative<std::string>(operand))
        return std::get<std::string>(operand);
    else
        return std::to_string(std::get<uint16_t>(operand));
}