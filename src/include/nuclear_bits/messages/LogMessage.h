#ifndef NUCLEAR_MESSAGES_LOGMESSAGE_H
#define NUCLEAR_MESSAGES_LOGMESSAGE_H
namespace NUClear {
namespace Messages {
    struct LogMessage {
        LogMessage(const std::string& message) : message(message) {}

        std::string message;
    };
}
}
#endif
