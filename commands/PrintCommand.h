#pragma once

#include "ICommand.h"
#include <string>
#include <optional>

class PrintCommand : public ICommand
{
public:
    PrintCommand(const std::string& message);

    CommandType getType() const override;

    void execute(Process& process) override;

    std::string toString() const override;

public:
    // Overload: message with one variable interpolated at the end.
    // Produces: "<message><varValue>"  e.g. "Value from: 42"
    PrintCommand(const std::string& message, const std::string& varName);

private:
    std::string              message;
    std::optional<std::string> varName; // if set, appends the variable's value
    
};