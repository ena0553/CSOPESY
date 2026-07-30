#include "SymbolTable.h"

bool SymbolTable::declare(const std::string& name, uint16_t value)
{
    if (table.find(name) != table.end())
    {
        return true; // already declared, DECLARE never overwrites
    }

    if (table.size() >= MAX_VARIABLES)
    {
        return false; // segment full (64 bytes / 32 vars) — ignore
    }

    table[name] = clamp(static_cast<int32_t>(value));
    return true;
}

bool SymbolTable::set(const std::string& name, int32_t value)
{
    auto it = table.find(name);
    if (it != table.end())
    {
        it->second = clamp(value);
        return true;
    }

    if (table.size() >= MAX_VARIABLES)
    {
        return false; // full — cannot auto-declare a new variable
    }

    table[name] = clamp(value);
    return true;
}

uint16_t SymbolTable::get(const std::string& name)
{
    auto it = table.find(name);
    if (it != table.end())
    {
        return it->second;
    }

    if (table.size() < MAX_VARIABLES)
    {
        table[name] = 0;
        return 0;
    }

    return 0; // full: treat as an undeclared read, but don't add it
}

bool SymbolTable::has(const std::string& name) const
{
    return table.find(name) != table.end();
}

bool SymbolTable::isFull() const
{
    return table.size() >= MAX_VARIABLES;
}

size_t SymbolTable::size() const
{
    return table.size();
}

uint16_t SymbolTable::clamp(int32_t value)
{
    if (value < 0)      return 0;
    if (value > 65535)  return 65535;
    return static_cast<uint16_t>(value);
}