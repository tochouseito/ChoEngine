#pragma once
#include"include/Core/EventCommand.h"
#include <string>

namespace Theatria::Core::Events
{
    // イベントの型宣言の置き場

    /*==================== System Init Events ====================*/

    /*==================== WinApp ====================*/

    /// @brief ウィンドウ表示イベント
    struct EveShowWindow
    {
        const char* msg{};
    };

} // namespace Theatria::Core::Events
