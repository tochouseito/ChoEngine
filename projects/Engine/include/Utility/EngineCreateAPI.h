#pragma once
#include "include/Main/Engine.h"
#include "include/Utility/APIExportsMacro.h"
namespace Theatria::API
{
//#ifdef ENGINECREATE_FUNCTION
    // Engineの生成
    THEATRIA_API Theatria::Engine* CreateEngine(RuntimeMode mode);
    // Engineの破棄
    THEATRIA_API void DestroyEngine(Theatria::Engine* engine);
    // ポインタを受け取る
    THEATRIA_API void SetEngine(Theatria::Engine* engine);
    // Engineのポインタ
    static Theatria::Engine* g_Engine = nullptr;
    // 稼働
    THEATRIA_API void RunEngine();
//#endif
}
