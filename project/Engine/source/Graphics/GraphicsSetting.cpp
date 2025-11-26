#include "pch.h"
#include "include/Graphics/GraphicsSetting.h"

namespace Theatria::Graphics::Setting
{
    uint32_t ResolutionWidth = 1280;    ///< 解像度幅
    uint32_t ResolutionHeight = 720;   ///< 解像度高さ

    const float kClearColor[4] = { 0.1f,0.25f,0.5f,1.0f }; ///< クリアカラー

    DXGI_FORMAT DefaultDXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    bool EnableVSync = true;          ///< VSync有効化フラグ
}
