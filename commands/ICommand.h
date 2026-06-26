#pragma once

#include <string>

class Process;

class ICommand {
    public:
        enum class CommandType {
            COMPUTE,
            IO,
            MEMORY,
            FILE,
        };

        virtual ~ICommand() = default; // virtual destructor for proper cleanup
        virtual CommandType getType() const = 0; // get command type
        virtual void execute(Process& process) = 0; // execute command on a process
        virtual std::string toString() const = 0; // string representation of the command

        // Every command counts as one instruction by default
        virtual int countCommands() const {
            return 1;
        }
};