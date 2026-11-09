#pragma once
#include <deque>
#include <mutex>
#include <string>

namespace Plugins::APC {
inline std::mutex messageMutex;
inline std::deque<std::string> waitingMessages;

inline void queueAPMessage(std::string message) {
    std::lock_guard<std::mutex> lock(messageMutex);
    waitingMessages.push_back(std::move(message));
    if (waitingMessages.size() > 64) waitingMessages.pop_front();
}

inline std::deque<std::string> takeAPMessages() {
    std::lock_guard<std::mutex> lock(messageMutex);
    std::deque<std::string> messages;
    messages.swap(waitingMessages);
    return messages;
}
}
