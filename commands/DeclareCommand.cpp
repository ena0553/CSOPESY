#include "DeclareCommand.h"
#include "../Process.h"

DeclareCommand::DeclareCommand(const std::string& varName, uint16_t value)
    : varName(varName), value(value)
{
}

ICommand::CommandType DeclareCommand::getType() const
{
    return CommandType::MEMORY;
}

void DeclareCommand::execute(Process& process)
{
    process.incrementCommandCounter();

    bool ok = process.getSymbolTable().declare(varName, value);
    if (ok)
    {
        process.log("DECLARE " + varName + " = " + std::to_string(value));
    }
    else
    {
        process.log("DECLARE " + varName + " ignored: symbol table full (max 32 variables)");
    }
}

std::string DeclareCommand::toString() const
{
    return "DECLARE " + varName + " = " + std::to_string(value);
}