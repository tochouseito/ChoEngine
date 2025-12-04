#pragma once
#ifndef NDEBUG
#include "include/Editor/BaseEditor.h"

namespace Theatria::Editor
{
    class Inspector final : public BaseEditor
    {
    public:
        Inspector() = default;
        ~Inspector() override = default;
        void Initialize() override;
        void Update() override;
    };
}
#endif // !NDEBUG
