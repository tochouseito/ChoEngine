#pragma once
#ifndef NDEBUG
#include "include/Editor/BaseEditor.h"

namespace Theatria::Editor
{
    class SceneView final : public BaseEditor
    {
    public:
        SceneView() = default;
        ~SceneView() override = default;
        void Initialize() override;
        void Update() override;
    };
}
#endif // !NDEBUG
