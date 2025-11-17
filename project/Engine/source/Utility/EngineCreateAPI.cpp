#include "pch.h"
#include "include/Utility/EngineCreateAPI.h"

THEATRIA_API Theatria::Engine* Theatria::API::CreateEngine(RuntimeMode mode)
{
    return new Theatria::Engine(mode);
}

THEATRIA_API void Theatria::API::DestroyEngine(Theatria::Engine* engine)
{
    delete engine;
}

THEATRIA_API void Theatria::API::SetEngine(Theatria::Engine* engine)
{
    g_Engine = engine;
}

// 稼働
THEATRIA_API void Theatria::API::RunEngine()
{
    if (g_Engine)
    {
        g_Engine->Operation();
    }
}
