#include "SymbolTable.h"

void SymbolTable::declare(const std::string& name, uint16_t value)
{
    // Only set if not already declared — DECLARE does not overwrite.
    if (table.find(name) == table.end())
    {
        table[name] = clamp(static_cast<int32_t>(value));
    }
}

void SymbolTable::set(const std::string& name, int32_t value)
{
    // Per spec: variables are auto-declared at 0 if not yet declared.
    table[name] = clamp(value);
}

uint16_t SymbolTable::get(const std::string& name)
{
    // Per spec: auto-declare at 0 if not yet declared.
    if (table.find(name) == table.end())
    {
        table[name] = 0;
    }
    return table[name];
}

bool SymbolTable::has(const std::string& name) const
{
    return table.find(name) != table.end();
}

uint16_t SymbolTable::clamp(int32_t value)
{
    if (value < 0)      return 0;
    if (value > 65535)  return 65535;
    return static_cast<uint16_t>(value);
}