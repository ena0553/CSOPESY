#pragma once

#include "ICommand.h"
#include <memory>
#include <vector>

class ForCommand : public ICommand {
public:
    ForCommand(int count, std::vector<std::shared_ptr<ICommand>> instructions);

    CommandType getType() const override;
    void execute(Process& process) override;
    std::string toString() const override;
private:
    int count;
    std::vector<std::shared_ptr<ICommand>> instructions;
};