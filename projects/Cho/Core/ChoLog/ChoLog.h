#pragma once
#define NOMINMAX // Windowのminmaxマクロを除外
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <mutex>
#include <chrono>
#include <ctime>
#include <variant>
#include <optional>
#include <source_location>
#include <system_error>

#define CHO_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::string fullMsg = "Assertion failed!\n\nMessage: " + std::string(msg); \
            MessageBoxA(nullptr, fullMsg.c_str(), "Assertion Error", MB_OK | MB_ICONERROR); \
            __debugbreak(); \
        } \
    } while (false)


std::wstring ConvertString(const std::string& str);
std::string ConvertString(const std::wstring& str);

namespace theatria
{

    enum class LogLevel
    {
        Info,
        Warn,
        Error,
        Debug,
        Assert
    };

    class Log
    {
    public:

        static void Write(
            LogLevel level,
            const std::variant<std::string, std::wstring>& message,
            std::optional<std::variant<bool, HRESULT>> flag = std::nullopt,
            const std::source_location location = std::source_location::current());
    private:
        static std::string LogFileName;
        static std::mutex logMutex;
        static void WriteToFile(const std::string& message);
        static std::string ToString(LogLevel level);
    };

}
