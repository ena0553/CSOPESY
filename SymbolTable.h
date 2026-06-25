#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

class SymbolTable
{
public:
    // declares a variable with an explicit initial value.
    // clamps value to [0, 65535]
    void declare(const std::string& name, uint16_t value = 0);

    // sets a variable and auto-declares at 0 if not yet declared.
    // clamps result to [0, 65535]
    void set(const std::string& name, int32_t value);

    // Returns the variable's value. Auto-declares at 0 if not yet declared.
    uint16_t get(const std::string& name);

    // Returns true if the variable has been declared.
    bool has(const std::string& name) const;

private:
    std::unordered_map<std::string, uint16_t> table;

    // clamps a signed 32-bit int to the uint16 range.
    static uint16_t clamp(int32_t value);
};