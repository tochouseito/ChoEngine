#pragma once
#ifndef NDEBUG
#include "include/Editor/BaseEditor.h"

namespace Theatria::Editor
{
    class AssetBrowser final : public BaseEditor
    {
    public:
        AssetBrowser() = default;
        ~AssetBrowser() override = default;
        void Initialize() override;
        void Update() override;
    };
}
#endif // !NDEBUG
