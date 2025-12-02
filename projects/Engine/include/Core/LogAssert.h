#pragma once
// === C++ Standard Library ===
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <format>
#include <source_location>
#include <fstream>
#include <iostream>
#include <optional>
#include <functional>
#include <variant>
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#include <DbgHelp.h>
#endif

namespace Theatria::Core
{
    /// @brief ログアサート
    class LogAssert final
    {
    public:
        /// @brief ログレベル
        enum class LogLevel : uint8_t
        {
            Info,
            Warn,
            Error,
            Fatal,
            Debug
        };

        /// @brief 出力先シンク種別
        enum class SinkKind : uint8_t
        {
            Console,
            VSOutput,
            File
        };

        /// @brief ログメッセージ
        struct LogMessage final
        {
            LogLevel level{};
            std::chrono::system_clock::time_point time;
            uint32_t thread_id{ 0 };
            std::string category; // "Render" など
            std::string file;
            std::string function;
            uint32_t line{ 0 };
            std::string text;
        };

    public:
        /// @brief ログレベルを文字列に変換
        static constexpr std::string_view ToString(LogLevel lv) noexcept
        {
            switch (lv)
            {
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warn:  return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default:              return "DEBUG";
            }
        }

        static bool ToBool(const std::variant<bool, HRESULT>& v) noexcept
        {
            return std::visit([](auto x) -> bool {
                if constexpr (std::is_same_v<std::decay_t<decltype(x)>, bool>)
                    return x;
                else
                    return SUCCEEDED(x); // HRESULT → bool
                }, v);
        }

        /*================ Print ================*/
        /// @brief 指定シンクにログ出力
        template<class... Args>
        static void Log(std::source_location loc, SinkKind sink, LogLevel level, std::string_view category,
            std::string_view fmt, Args&&... args)
        {
            auto msg = BuildMessage(level, category, fmt, loc, std::forward<Args>(args)...);
            DispatchToSink(sink, msg);
        }
        /// @brief 指定シンク群にログ出力
        template<class... Args>
        static void LogTo(std::source_location& loc, const std::initializer_list<SinkKind>& sinks, LogLevel level, std::string_view category,
            std::string_view fmt, Args&&... args)
        {
            auto msg = BuildMessage(level, category, fmt, loc, std::forward<Args>(args)...);
            for (auto s : sinks) DispatchToSink(s, msg);
        }
        // @brief 内部ブリッジ
        template<class... Args>
        static void LogToWithLoc(const std::initializer_list<SinkKind>& sinks, LogLevel level,
            std::string_view category, const std::source_location& loc,
            std::string_view fmt, Args&&... args)
        {
            auto msg = BuildMessage(level, category, fmt, loc, std::forward<Args>(args)...);
            for (auto s : sinks) DispatchToSink(s, msg);
        }

        /*================ Assert ================*/
        /// @brief VERIFY: 常に評価、失敗はログ（継続）
        template<class... Args>
        static bool Verify(std::variant<bool, HRESULT> expr, std::string_view category, std::string_view fmt_on_fail = "VERIFY failed: {}",
            std::string_view expr_str = {}, const std::initializer_list<SinkKind>& sinks = { SinkKind::Console, SinkKind::VSOutput },
            const std::source_location& loc = std::source_location::current(), Args&&... args)
        {
            if (ToBool(expr)) return true;
            auto msg = expr_str.empty() ? std::vformat(fmt_on_fail, std::make_format_args(std::forward<Args>(args)...))
                : std::format("{} ({})", fmt_on_fail, expr_str);
            LogToWithLoc(sinks, LogLevel::Error, category, loc, "{}", msg);
            MBox(BuildFailMessage(LogLevel::Error, category, msg, loc));
            return false;
        }

        /// @brief ENSURE: 失敗ログ＋通知（継続）
        static void Ensure(std::variant<bool, HRESULT> expr, std::string_view category, std::string_view reason = {},
            const std::initializer_list<SinkKind>& sinks = { SinkKind::Console, SinkKind::VSOutput },
            const std::source_location& loc = std::source_location::current())
        {
            if (ToBool(expr)) return;
            auto m = BuildFailMessage(LogLevel::Error, category,
                reason.empty() ? std::string("ENSURE failed") : std::string(reason), loc);
            for (auto s : sinks) DispatchToSink(s, m);
        }

        /// @brief CHECK: 失敗ログ＋通知＋ブレーク（中断）
        static void Check(std::variant<bool, HRESULT> expr, std::string_view category, std::string_view reason = {},
            const std::initializer_list<SinkKind>& sinks = { SinkKind::Console, SinkKind::VSOutput },
            const std::source_location& loc = std::source_location::current())
        {
            if (ToBool(expr)) return;
            auto m = BuildFailMessage(LogLevel::Fatal, category,
                reason.empty() ? std::string("CHECK failed") : std::string(reason), loc);
            for (auto s : sinks) DispatchToSink(s, m);
            MBox(m);
            DebugBreak();
        }

        ///*================ Getter / Clear ================*/
        //// Console
        //const std::vector<LogMessage> GetConsoleMessages() const
        //{
        //    std::lock_guard lock(m_ConsoleMutex);
        //    return m_ConsoleMessages;
        //}
        //void ClearConsoleMessages(){
        //    std::lock_guard lock(m_ConsoleMutex);
        //    m_ConsoleMessages.clear();
        //}
        //// VSOutput
        //const std::vector<LogMessage> GetVSOutputMessages() const
        //{
        //    std::lock_guard lock(m_VSOutputMutex);
        //    return m_VSOutputMessages;
        //}
        //void ClearVSOutputMessages(){
        //    std::lock_guard lock(m_VSOutputMutex);
        //    m_VSOutputMessages.clear();
        //}
        //// File
        //const std::vector<LogMessage> GetFileMessages() const
        //{
        //    std::lock_guard lock(m_FileMutex);
        //    return m_FileMessages;
        //}
        //void ClearFileMessages()
        //{
        //    std::lock_guard lock(m_FileMutex);
        //    m_FileMessages.clear();
        //}
        //// Clear All
        //void ClearAllMessages()
        //{
        //    ClearConsoleMessages();
        //    ClearVSOutputMessages();
        //    ClearFileMessages();
        //}
    private:
        /// @brief ログメッセージ構築
        /// @tparam ...Args 
        /// @param level 
        /// @param category 
        /// @param fmt 
        /// @param loc 
        /// @param ...args 
        /// @return 
        template<class... Args>
        static LogMessage BuildMessage(LogLevel level, std::string_view category,
            std::string_view fmt, const std::source_location& loc, Args&&... args)
        {
            LogMessage m;
            m.level = level;
            m.time = std::chrono::system_clock::now();
            m.thread_id = ::GetCurrentThreadId();
            m.category = std::string(category);
            m.file = loc.file_name();
            m.function = loc.function_name();
            m.line = loc.line();
            m.text = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
            return m;
        }

        /// @brief 
        /// @param level 
        /// @param category 
        /// @param reason 
        /// @param loc 
        /// @param capture_stack 
        /// @return 
        static LogMessage BuildFailMessage(LogLevel level, std::string_view category, std::string reason,
            const std::source_location& loc)
        {
            LogMessage m;
            m.level = level;
            m.time = std::chrono::system_clock::now();
            m.thread_id = ::GetCurrentThreadId();
            m.category = std::string(category);
            m.file = loc.file_name();
            m.function = loc.function_name();
            m.line = loc.line();
            m.text = std::move(reason);
            return m;
        }

        /*================ 各シンク処理 ================*/
        static void PushConsole(const LogMessage& m)
        {
            std::lock_guard lock(m_ConsoleMutex);
            //m_ConsoleMessages.push_back(m);
            std::cout << m.text << std::endl;
        }
        static void PushVSOut(const LogMessage& m)
        {
            std::lock_guard lock(m_VSOutputMutex);
            //m_VSOutputMessages.push_back(m);
            OutputDebugStringA((m.text + "\n").c_str());

        }
        static void PushFile(const LogMessage&)
        {
            std::lock_guard lock(m_FileMutex);
            //m_FileMessages.push_back(m);
        }

        /// @brief 
        /// @param s 
        /// @param m 
        static void DispatchToSink(SinkKind s, const LogMessage& m)
        {
            switch (s)
            {
            case SinkKind::Console:   PushConsole(m);   break;
            case SinkKind::VSOutput:  PushVSOut(m);     break;
            case SinkKind::File:      PushFile(m);      break;
            }
        }

        static void MBox(const LogMessage& m)
        {
            std::string fullMsg = "Assertion failed!\n\nMessage: " + m.text;
            MessageBoxA(nullptr, fullMsg.c_str(), "Assertion Error", MB_OK | MB_ICONERROR);
        }

        static void DebugBreak()
        {
            __debugbreak();
        }

        // Console
        inline static std::mutex m_ConsoleMutex;
        // std::vector<LogMessage> m_ConsoleMessages;

        // VSOutput
        inline static std::mutex m_VSOutputMutex;
        // std::vector<LogMessage> m_VSOutputMessages;

        // File
        inline static std::mutex m_FileMutex;
        // std::vector<LogMessage> m_FileMessages;
    };
};
