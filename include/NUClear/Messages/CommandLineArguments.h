#ifndef NUCLEAR_MESSAGES_COMMANDLINEMESSAGE_H
#define NUCLEAR_MESSAGES_COMMANDLINEMESSAGE_H
namespace NUClear {
namespace Messages {
    struct CommandLineArguments {
        CommandLineArguments(const std::vector<std::string>& args) : args(args) {}

        std::vector<std::string> args;
    };
}
}
#endif
