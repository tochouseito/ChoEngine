#pragma once
#ifndef NDEBUG
#include "include/Editor/BaseEditor.h"

namespace Theatria::Editor
{
    class GameView final : public BaseEditor
    {
    public:
        GameView() = default;
        ~GameView() override = default;
        void Initialize() override;
        void Update() override;
    };
}
#endif // !NDEBUG
