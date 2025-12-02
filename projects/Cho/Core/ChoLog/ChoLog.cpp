#include "ChoLog.h"
#include <cassert>
#include <string>

std::string theatria::Log::LogFileName = "TheatriaEngineLog.txt";
std::mutex theatria::Log::logMutex;

std::wstring ConvertString(const std::string& s)
{
    if (s.empty()) return {};
    int size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        s.c_str(), static_cast<int>(s.size()),
        nullptr, 0);
    if (size <= 0) return {};
    std::wstring ws(size, L'\0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        s.c_str(), static_cast<int>(s.size()),
        ws.data(), size);
    return ws;
}

std::string ConvertString(const std::wstring& ws)
{
    if (ws.empty()) return {};
    // 無効文字検出を有効化
    int size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        ws.c_str(), static_cast<int>(ws.size()),
        nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string utf8(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        ws.c_str(), static_cast<int>(ws.size()),
        utf8.data(), size, nullptr, nullptr);
    return utf8;
}

void theatria::Log::Write(
    LogLevel level,
    const std::variant<std::string, std::wstring>& message,
    std::optional<std::variant<bool, HRESULT>> flag,
    const std::source_location location)
{
    // --- メッセージを std::string に正規化 ---
    std::string msg;
    if (std::holds_alternative<std::string>(message))
    {
        msg = std::get<std::string>(message);
    }
    else
    {
        const std::wstring& wmsg = std::get<std::wstring>(message);
        // UTF-8 に変換
		msg = ConvertString(wmsg);
    }

    std::string suffix;

    if (flag.has_value())
    {
        if (std::holds_alternative<bool>(*flag))
        {
            bool b = std::get<bool>(*flag);
            suffix = b ? " : Succeeded" : " : Failed";

            if (level == LogLevel::Assert)
            {
                if (!b)
                {
                    std::string logMessage = std::format(
                        "[ASSERT] {}:{} -\n {}{}",
                        location.file_name(), location.line(), msg, suffix);
#ifdef _DEBUG
                    OutputDebugStringA((logMessage + "\n").c_str());
#endif
#ifdef NDEBUG
                    //WriteToFile(fullMessage);
#endif
                    CHO_ASSERT(false, logMessage.c_str());
                    return;
                }
                level = LogLevel::Info; // 成功ならInfo扱い
            }
        }
        else if (std::holds_alternative<HRESULT>(*flag))
        {
            HRESULT hr = std::get<HRESULT>(*flag);
            bool success = SUCCEEDED(hr);
            suffix = success ? " : Succeeded"
                : std::format(" : Failed (HRESULT: 0x{:08X})", static_cast<unsigned int>(hr));

            if (level == LogLevel::Assert)
            {
                if (FAILED(hr))
                {
                    std::string logMessage = std::format(
                        "[ASSERT] {}:{} -\n {}{}",
                        location.file_name(), location.line(), msg, suffix);
#ifdef _DEBUG
                    OutputDebugStringA((logMessage + "\n").c_str());
#endif
#ifdef NDEBUG
                    //WriteToFile(fullMessage);
#endif
                    CHO_ASSERT(false, logMessage.c_str());
                    return;
                }
                level = LogLevel::Info;
            }
        }
    }
    else
    {
        if (level == LogLevel::Assert)
        {
            std::string logMessage = std::format(
                "[ASSERT] {}:{} -\n {}",
                location.file_name(), location.line(), msg);
#ifdef _DEBUG
            OutputDebugStringA((logMessage + "\n").c_str());
#endif
#ifdef NDEBUG
            //WriteToFile(fullMessage);
#endif
            CHO_ASSERT(false, logMessage.c_str());
            return;
        }
    }

    std::string prefix = std::format("[{}] {}:{} -\n ",
        ToString(level), location.file_name(), location.line());
    std::string fullMessage = prefix + msg + suffix;

#ifdef _DEBUG
    OutputDebugStringA((fullMessage + "\n").c_str());
#endif
#ifdef NDEBUG
    //WriteToFile(fullMessage);
#endif
}

void theatria::Log::WriteToFile(const std::string& message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream logFile("TheatriaEngineLog.txt", std::ios::app);
    if (logFile.is_open())
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm localTime{};
        localtime_s(&localTime, &now);

        logFile << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] "
            << message << std::endl;
    }
}

std::string theatria::Log::ToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Assert: return "ASSERT";
    default: return "UNKNOWN";
    }
}
