#pragma once
#ifndef NDEBUG
#include "include/Editor/BaseEditor.h"

namespace Theatria
{
    namespace Graphics
    {
        class DescriptorAllocator;
        class FrameGraph;
    }
    namespace Editor
    {
        class SceneView final : public BaseEditor
        {
        public:
            SceneView(Graphics::DescriptorAllocator* da, Graphics::FrameGraph* fg)
                : m_DescriptorAllocator(da), m_FrameGraph(fg) {}
            ~SceneView() override = default;
            void Initialize() override;
            void Update() override;
        private:
            Graphics::DescriptorAllocator* m_DescriptorAllocator = nullptr;
            Graphics::FrameGraph* m_FrameGraph = nullptr;
        };
    }
}

#endif // !NDEBUG
