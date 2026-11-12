#pragma once

#include <string>
#include <tuple>
#include <charconv>
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
    bool TryParse(const std::vector<std::string_view>& parts) {
        size_t index = 1; // Skip message type
        bool success = true;

        std::apply([&](auto&... f) {
            ((success = success && ParseField(parts, index++, f)), ...);
        }, static_cast<DerivedType&>(*this).Fields());

        return success;
    }

private:

    template <typename T>
    static bool ParseField(const std::vector<std::string_view>& parts, size_t i, T& field) {
        if (i >= parts.size()) 
            return false;

        const auto& s = parts[i];
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), field);
        
        return ec == std::errc{} && ptr == s.data() + s.size();
    }

    static bool ParseField(const std::vector<std::string_view>& parts, size_t i, std::string& field) {
        if (i >= parts.size()) 
            return false;

        try {
            field = std::string(parts[i]);
        } 
        catch (...) {
            return false;
        }

        return true;
    }
};

inline std::vector<std::string_view> split(std::string_view s, char delim) {
    std::vector<std::string_view> result;
    size_t start = 0, pos;
    while ((pos = s.find(delim, start)) != std::string_view::npos) {
        result.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    result.push_back(s.substr(start));
    return result;
}

template <typename MessageType>
void splitMessage(const std::string& message, std::vector<std::string_view>& parts, MessageType& messageType) {
    parts = split(message, fieldSeparator);
    messageType = (MessageType)std::stoi(parts[0].data());
}