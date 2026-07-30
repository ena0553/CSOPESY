#include "MemoryCommand.h"
#include "../MemoryManager.h"
#include "../Process.h"

#include <sstream>
#include <iomanip>
#include <iostream>

MemoryCommand::MemoryCommand(Operation operation,
                               MemoryManager* memoryManager,
                               uint32_t address,
                               const std::string& variable,
                               Operand source)
    : operation(operation),
      memoryManager(memoryManager),
      address(address),
      variable(variable),
      source(source)
{
}

ICommand::CommandType MemoryCommand::getType() const {
    return CommandType::MEMORY;
}

void MemoryCommand::execute(Process& process) {
    try {
        if (operation == Operation::READ) {
            uint16_t value = memoryManager->read(&process, address);
            bool stored = process.getSymbolTable().set(variable, value);
            if (stored) {
                process.log("READ from address " + std::to_string(address) + ": " + std::to_string(value) + " into variable " + variable);
            } else {
                process.log("READ from address " + std::to_string(address) + " ignored: symbol table full (max 32 variables)");
            }
        } else {
            uint16_t valueToWrite = resolve(source, process);
            memoryManager->write(&process, address, valueToWrite);
            process.log("WRITE to address " + std::to_string(address) + ": " + std::to_string(valueToWrite));
        }
        process.incrementCommandCounter();
    } catch (const std::exception& e) {
        process.log("Memory operation failed: " + std::string(e.what()));
    }
}

std::string MemoryCommand::toString() const {
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::hex << address;
    if (operation == Operation::READ) {
        return "READ from address " + ss.str() + " into variable " + variable;
    } else {
        return "WRITE to address " + ss.str() + ": " + operandStr(source);
    }
}

uint16_t MemoryCommand::resolve(const Operand& operand, Process& process) const
{
    if (std::holds_alternative<std::string>(operand))
        return process.getSymbolTable().get(std::get<std::string>(operand));
    else
        return std::get<uint16_t>(operand);
}

std::string MemoryCommand::operandStr(const Operand& operand) const
{
    if (std::holds_alternative<std::string>(operand))
        return std::get<std::string>(operand);
    else
        return std::to_string(std::get<uint16_t>(operand));
}