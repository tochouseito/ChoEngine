#pragma once
#ifndef NDEBUG
#include <string>
#include <memory>

#include "include/Editor/AssetBrowser.h"
#include "include/Editor/GameView.h"
#include "include/Editor/SceneView.h"
#include "include/Editor/Hierarchy.h"
#include "include/Editor/Inspector.h"

namespace Theatria
{
    namespace Core
    {
        class FrameCounter;
    }
    namespace Graphics
    {
        class DescriptorAllocator;
        class FrameGraph;
    }
    namespace Editor
    {
        class EditorManager final
        {
        public:
            bool Initialize(Core::FrameCounter* fc,Graphics::DescriptorAllocator* da, Graphics::FrameGraph* fg);
            void Shutdown();
            void Update();

        private:
            void BackDockingWindows();

            Core::FrameCounter* m_pFrameCounter = nullptr;

            std::unique_ptr<AssetBrowser> m_AssetBrowser;
            std::unique_ptr<GameView> m_GameView;
            std::unique_ptr<SceneView> m_SceneView;
            std::unique_ptr<Hierarchy> m_Hierarchy;
            std::unique_ptr<Inspector> m_Inspector;
        };
    }
}
#endif // !NDEBUG

