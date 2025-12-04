#pragma once
#ifndef NDEBUG
#include "include/Editor/BaseEditor.h"

namespace Theatria::Editor
{
    class Hierarchy final : public BaseEditor
    {
    public:
        Hierarchy() = default;
        ~Hierarchy() override = default;
        void Initialize() override;
        void Update() override;
    };
}
#endif // !NDEBUG
