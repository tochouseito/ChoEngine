#pragma once
#include <cstdint>
#include <dxgiformat.h>

namespace Theatria::Graphics::Setting
{
    /// @brief グラフィックス設定
    extern uint32_t ResolutionWidth;    ///< 解像度幅
    extern uint32_t ResolutionHeight;   ///< 解像度高さ

    extern const float kClearColor[4]; ///< クリアカラー

    extern DXGI_FORMAT DefaultDXGIFormat;

    extern bool EnableVSync;          ///< VSync有効化フラグ
}
