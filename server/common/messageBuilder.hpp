#pragma once

#include <string>

class MessageBuilder {
    public:
        const char fieldSeparator = ';';
        const char messageEndChar = '~';

        MessageBuilder(int messageTypesCount);

        template <typename ...Args>
        const std::string createMessage(int messageType, Args... args) const{
            if(messageType >= messageTypesCount){
                printf("To high message type index");
                return "";
            }
            return (std::to_string(messageType) + ... + (fieldSeparator + toStr(args))) + messageEndChar;
        }

    private:

        const int messageTypesCount;

        template <typename T>
        std::string toStr(const T& value) const {
            if constexpr (std::is_convertible_v<T, std::string>) {
                return value;
            } else {
                return std::to_string(value);
            }
        }
};
