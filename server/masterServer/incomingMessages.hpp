// #pragma once

// #include "common/messages.hpp"

// namespace incoming {

//     enum class Type {
//         Init,
//         StartGame
//     };

//     struct TestMessage : IncomingMessage<TestMessage> {
//         int id = 0;
//         std::string name;

//         auto Fields() { return std::tie(id, name); }
//     };
// }