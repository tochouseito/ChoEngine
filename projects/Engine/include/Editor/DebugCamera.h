#pragma once
#ifndef NDEBUG

namespace Theatria::Editor
{
    class DebugCamera final
    {
    public:
        DebugCamera() = default;
        ~DebugCamera() = default;
        void Initialize();
        void Update();
    };
}
#endif // !NDEBUG
