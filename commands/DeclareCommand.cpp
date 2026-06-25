#include "DeclareCommand.h"
#include "../Process.h"

DeclareCommand::DeclareCommand(const std::string& varName, uint16_t value)
    : varName(varName), value(value)
{
}

ICommand::CommandType DeclareCommand::getType() const
{
    return CommandType::COMPUTE;
}

void DeclareCommand::execute(Process& process)
{
    // declare() only writes if the variable does not already exist.
    process.getSymbolTable().declare(varName, value);

    process.log("DECLARE " + varName + " = " + std::to_string(value));
}

std::string DeclareCommand::toString() const
{
    return "DECLARE " + varName + " = " + std::to_string(value);
}