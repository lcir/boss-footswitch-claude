#pragma once
#include <Arduino.h>
#include "footswitch.h"

// ── Log entry ─────────────────────────────────────────────────────────────────
enum class LogLevel : uint8_t { INFO, OK, WARN, ERR };

struct LogEntry {
    uint32_t  ms;
    LogLevel  level;
    char      msg[80];
};

class WebInterface {
public:
    static constexpr uint8_t LOG_BUF_SIZE = 30;

    void begin();
    void broadcastState();
    void log(LogLevel level, const char* fmt, ...);
    void handleWsMessage(const char* msg, size_t len);

    // Accessible by WS connect handler in .cpp
    LogEntry _logBuf[LOG_BUF_SIZE];
    uint8_t  _logHead  = 0;
    uint8_t  _logCount = 0;

private:
    void setupRoutes();
    void broadcastLog(const LogEntry& entry);
};

extern WebInterface WebUI;
