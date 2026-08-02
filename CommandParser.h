#pragma once

#include <string>
#include <vector>
#include <memory>
#include "commands/ICommand.h"
#include "commands/AddCommand.h"   // for Operand alias

class MemoryManager;

// Parses a user-supplied instruction string (used by "screen -c") into ICommand objects.
//
// Supported syntax (semicolon-separated top-level instructions, case-sensitive keywords):
//   DECLARE varName, value
//   ADD dest op1 op2
//   SUBTRACT dest op1 op2
//   PRINT("literal text")
//   PRINT("literal text" + varName)
//   SLEEP ticks                      -- ticks: 0-255
//   READ varName address             -- address: decimal or 0x hex
//   WRITE address source             -- source: variable name or literal
//   FOR count [instr1; instr2; ...]   -- nestable up to 3 levels deep
//
// Operands (op1/op2/source) may be a literal decimal (0-65535) or a variable name.
// Throws std::runtime_error with a descriptive message on malformed input.
class CommandParser
{
public:
    static std::vector<std::shared_ptr<ICommand>> parse(const std::string& instructionText,
                                                          long long memoryUsage,
                                                          MemoryManager* memManager);

private:
    static std::vector<std::string> splitTopLevel(const std::string& text, char delim);
    static std::shared_ptr<ICommand> parseOne(const std::string& instr, int depth,
                                               long long memoryUsage, MemoryManager* memManager);
    static Operand parseOperand(std::string token);
    static std::string trim(const std::string& s);
};