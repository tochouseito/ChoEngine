#pragma once
#include "include/Core/EventCommand.h"
#include <string>

#include "include/Platform/WinApp.h"
#include "include/Core/LogAssert.h"

namespace Theatria::Core::Commands
{
    // コマンドの型宣言の置き場

    /*==================== WinApp ====================*/

    struct CmdShowWindow
    {
        char msg[256]{};
    };
    inline void ExecShowWindow(void*, const void* data)
    {
        const auto* cmd = static_cast<const CmdShowWindow*>(data);
         Theatria::Core::LogAssert::Log(std::source_location::current(), Theatria::Core::LogAssert::SinkKind::Console,
            Theatria::Core::LogAssert::LogLevel::Info,
            "WinApp",
            "Executing CmdShowWindow: {}", cmd->msg);
        Theatria::Platform::WinApp::ShowWindowApp();
    }

} // namespace Theatria::Core::Commands
