#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

class SymbolTable
{
public:
    // Fixed-size symbol table segment: 64 bytes total, 2 bytes per uint16 variable => 32 variables max.
    static constexpr size_t SEGMENT_SIZE_BYTES = 64;
    static constexpr size_t MAX_VARIABLES = SEGMENT_SIZE_BYTES / sizeof(uint16_t); // 32

    // Declares a variable with an explicit initial value. Does not overwrite an existing variable.
    // If 'name' is new and the table already holds MAX_VARIABLES entries, the declaration is ignored.
    // Returns false only when ignored due to the limit.
    bool declare(const std::string& name, uint16_t value = 0);

    // Sets a variable, auto-declaring it at 0 first if needed.
    // If the variable is new and the table is full, the write is ignored (returns false).
    bool set(const std::string& name, int32_t value);

    // Returns the variable's value. Auto-declares at 0 if not yet declared and space remains;
    // if the table is full and the variable doesn't exist, returns 0 without declaring it.
    uint16_t get(const std::string& name);

    bool has(const std::string& name) const;
    bool isFull() const;
    size_t size() const;

private:
    std::unordered_map<std::string, uint16_t> table;
    static uint16_t clamp(int32_t value);
};