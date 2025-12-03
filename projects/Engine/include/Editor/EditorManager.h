#pragma once
#ifndef NDEBUG

namespace Theatria
{
    namespace Core
    {
        class FrameCounter;
    }
    namespace Editor
    {
        class EditorManager final
        {
        public:
            bool Initialize(Core::FrameCounter* fc);
            void Shutdown();
            void Update();

        private:
            void BackDockingWindows();

            Core::FrameCounter* m_pFrameCounter = nullptr;
        };
    }
}
#endif // !NDEBUG

