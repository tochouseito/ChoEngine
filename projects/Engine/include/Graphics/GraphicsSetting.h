#pragma once
#include <cstdint>
#include <string>
#include <dxgiformat.h>

namespace Theatria::Graphics::Setting
{
    /// @brief グラフィックス設定
    extern uint32_t ResolutionWidth;    ///< 解像度幅
    extern uint32_t ResolutionHeight;   ///< 解像度高さ

    constexpr uint32_t kMaxBufferingCount = 3; ///< 最大バッファリング数
    extern uint32_t BufferingCount; ///< バッファリング数
    extern uint32_t DisplayRefreshrate;          ///< 最大FPS(モニターのリフレッシュレート)

    extern const float kClearColor[4]; ///< クリアカラー
    extern DXGI_FORMAT DefaultDXGIFormat;
    extern bool EnableVSync;          ///< VSync有効化フラグ
    extern std::string ShaderCacheDirectory; ///< シェーダーキャッシュディレクトリ
}
