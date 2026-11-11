#pragma once

#include <string>
#include <tuple>
#include <sstream>
#include <vector>

const char fieldSeparator = ';';
const char recordSeparator = '!';

template <typename DerivedType>
struct OutgoingMessage {
    std::string Build() const {
        std::string message;

        if constexpr (requires { DerivedType::MessageType; }) {
            message += std::to_string(static_cast<int>(DerivedType::MessageType)) + fieldSeparator;
        }

        std::apply([&](const auto&... f) {
            ((message += toStr(f) + fieldSeparator), ...);
        }, static_cast<const DerivedType&>(*this).Fields());

        message.pop_back();

        return message;
    }

private:
    template <typename T>
    static std::string toStr(const T& value) {
        return std::to_string(value);
    }

    static std::string toStr(const std::string& value) {
        return value;
    }
};

template <typename DerivedType, typename RecordType>
struct OutgoingMultilineMessage {
    std::string Build() const {
        std::string message = std::to_string(static_cast<int>(DerivedType::MessageType)) + fieldSeparator;

        for (const auto& record : records) {
            message += record.Build() + recordSeparator;
        }
        
        if(message.size() > 0) {
            message.pop_back(); 
        }

        return message;
    }

    void addRecord(const RecordType& record) {
        records.push_back(record);
    }

    private:
        std::vector<RecordType> records;
};

template <typename DerivedType>
struct IncomingMessage {
    void Parse(const std::string& message) {
        std::istringstream is(message);
        std::apply([&](auto&... f) {
            (ParseField(is, f), ...);
        }, static_cast<DerivedType&>(*this).Fields());
    }

private:
    template <typename T>
    static void ParseField(std::istringstream& is, T& field) {
        std::string token;
        std::getline(is, token, fieldSeparator);
        std::istringstream(token) >> field;
    }

    static void ParseField(std::istringstream& is, std::string& field) {
        std::getline(is, field, fieldSeparator);  
    }
};
