#include "CommandParser.h"
#include "commands/PrintCommand.h"
#include "commands/DeclareCommand.h"
#include "commands/AddCommand.h"
#include "commands/SubtractCommand.h"
#include "commands/SleepCommand.h"
#include "commands/ForCommand.h"
#include "commands/MemoryCommand.h"

#include <stdexcept>
#include <cctype>

std::string CommandParser::trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// Splits on 'delim' but ignores delimiters inside (), [], or "quotes" so nested
// FOR bodies and PRINT strings survive a top-level split.
std::vector<std::string> CommandParser::splitTopLevel(const std::string& text, char delim)
{
    std::vector<std::string> parts;
    int depth = 0;
    bool inQuotes = false;
    std::string current;

    for (char c : text)
    {
        if (c == '"') inQuotes = !inQuotes;

        if (!inQuotes)
        {
            if (c == '(' || c == '[') depth++;
            else if (c == ')' || c == ']') depth--;
        }

        if (c == delim && depth == 0 && !inQuotes)
        {
            parts.push_back(trim(current));
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    std::string last = trim(current);
    if (!last.empty()) parts.push_back(last);

    return parts;
}

Operand CommandParser::parseOperand(std::string token)
{
    token = trim(token);
    if (token.empty())
        throw std::runtime_error("Empty operand in instruction");

    bool isNumber = true;
    for (char c : token) { if (!isdigit(static_cast<unsigned char>(c))) { isNumber = false; break; } }

    if (isNumber)
    {
        long val = std::stol(token);
        if (val < 0 || val > 65535)
            throw std::runtime_error("Literal value out of uint16 range: " + token);
        return static_cast<uint16_t>(val);
    }

    return token; // variable name
}

std::shared_ptr<ICommand> CommandParser::parseOne(const std::string& instrRaw, int depth,
                                                     long long memoryUsage, MemoryManager* memManager)
{
    std::string instr = trim(instrRaw);
    if (instr.empty())
        throw std::runtime_error("Empty instruction");

    size_t openParen = instr.find('(');
    if (openParen == std::string::npos || instr.back() != ')')
        throw std::runtime_error("Malformed instruction: " + instr);

    std::string keyword = trim(instr.substr(0, openParen));
    std::string argsStr = instr.substr(openParen + 1, instr.size() - openParen - 2);

    if (keyword == "PRINT")
    {
        std::string args = trim(argsStr);
        size_t firstQuote = args.find('"');
        size_t secondQuote = args.find('"', firstQuote + 1);
        if (firstQuote == std::string::npos || secondQuote == std::string::npos)
            throw std::runtime_error("PRINT requires a quoted string: " + instr);

        std::string message = args.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        std::string rest = trim(args.substr(secondQuote + 1));

        if (rest.empty())
            return std::make_shared<PrintCommand>(message);

        if (rest[0] != '+')
            throw std::runtime_error("PRINT: expected '+' before variable name: " + instr);

        std::string varName = trim(rest.substr(1));
        if (varName.empty())
            throw std::runtime_error("PRINT: missing variable name after '+': " + instr);

        return std::make_shared<PrintCommand>(message, varName);
    }

    if (keyword == "DECLARE")
    {
        auto parts = splitTopLevel(argsStr, ',');
        if (parts.size() != 2)
            throw std::runtime_error("DECLARE requires 2 arguments (name, value): " + instr);

        long val = std::stol(trim(parts[1]));
        if (val < 0 || val > 65535)
            throw std::runtime_error("DECLARE value out of range: " + instr);

        return std::make_shared<DeclareCommand>(trim(parts[0]), static_cast<uint16_t>(val));
    }

    if (keyword == "ADD" || keyword == "SUBTRACT")
    {
        auto parts = splitTopLevel(argsStr, ',');
        if (parts.size() != 3)
            throw std::runtime_error(keyword + " requires 3 arguments (dest, op1, op2): " + instr);

        std::string dest = trim(parts[0]);
        Operand op1 = parseOperand(parts[1]);
        Operand op2 = parseOperand(parts[2]);

        if (keyword == "ADD")
            return std::make_shared<AddCommand>(dest, op1, op2);
        else
            return std::make_shared<SubtractCommand>(dest, op1, op2);
    }

    if (keyword == "SLEEP")
    {
        long ticks = std::stol(trim(argsStr));
        if (ticks < 0 || ticks > 255)
            throw std::runtime_error("SLEEP ticks must be 0-255: " + instr);
        return std::make_shared<SleepCommand>(static_cast<uint8_t>(ticks));
    }

    if (keyword == "READ")
    {
        auto parts = splitTopLevel(argsStr, ',');
        if (parts.size() != 2)
            throw std::runtime_error("READ requires 2 arguments (varName, address): " + instr);

        std::string varName = trim(parts[0]);
        uint32_t address = static_cast<uint32_t>(std::stoul(trim(parts[1]), nullptr, 0));

        if (static_cast<long long>(address) + 2 > memoryUsage)
            throw std::runtime_error("READ address out of bounds for this process: " + instr);

        return std::make_shared<MemoryCommand>(MemoryCommand::Operation::READ, memManager, address, varName, Operand{});
    }

    if (keyword == "WRITE")
    {
        auto parts = splitTopLevel(argsStr, ',');
        if (parts.size() != 2)
            throw std::runtime_error("WRITE requires 2 arguments (address, source): " + instr);

        uint32_t address = static_cast<uint32_t>(std::stoul(trim(parts[0]), nullptr, 0));
        Operand source = parseOperand(parts[1]);

        if (static_cast<long long>(address) + 2 > memoryUsage)
            throw std::runtime_error("WRITE address out of bounds for this process: " + instr);

        return std::make_shared<MemoryCommand>(MemoryCommand::Operation::WRITE, memManager, address, "", source);
    }

    if (keyword == "FOR")
    {
        if (depth >= 3)
            throw std::runtime_error("FOR nesting exceeds max depth of 3: " + instr);

        size_t openBracket = argsStr.find('[');
        size_t closeBracket = argsStr.rfind(']');
        if (openBracket == std::string::npos || closeBracket == std::string::npos || closeBracket < openBracket)
            throw std::runtime_error("FOR requires a bracketed instruction list: " + instr);

        std::string body = argsStr.substr(openBracket + 1, closeBracket - openBracket - 1);
        std::string afterBracket = trim(argsStr.substr(closeBracket + 1));

        if (afterBracket.empty() || afterBracket[0] != ',')
            throw std::runtime_error("FOR requires a repeat count after the instruction list: " + instr);

        long count = std::stol(trim(afterBracket.substr(1)));
        if (count < 1)
            throw std::runtime_error("FOR repeat count must be >= 1: " + instr);

        auto bodyInstrs = splitTopLevel(body, ';');
        if (bodyInstrs.empty())
            throw std::runtime_error("FOR body must contain at least one instruction: " + instr);

        std::vector<std::shared_ptr<ICommand>> instructions;
        for (auto& bi : bodyInstrs)
            instructions.push_back(parseOne(bi, depth + 1, memoryUsage, memManager));

        return std::make_shared<ForCommand>(static_cast<int>(count), instructions);
    }

    throw std::runtime_error("Unknown instruction keyword: " + keyword);
}

std::vector<std::shared_ptr<ICommand>> CommandParser::parse(const std::string& instructionText,
                                                              long long memoryUsage,
                                                              MemoryManager* memManager)
{
    auto topLevel = splitTopLevel(instructionText, ';');

    if (topLevel.empty())
        throw std::runtime_error("Instruction list is empty");

    if (topLevel.size() > 50)
        throw std::runtime_error("Instruction count must be between 1 and 50 (got " + std::to_string(topLevel.size()) + ")");

    std::vector<std::shared_ptr<ICommand>> result;
    for (auto& instr : topLevel)
        result.push_back(parseOne(instr, 0, memoryUsage, memManager));

    return result;
}