#pragma once
#ifndef NDEBUG

namespace Theatria
{
    namespace Graphics
    {
        class RenderDevice;
        class DescriptorAllocator;
        class CommandContext;
    };

    namespace Editor
    {
        class ImGuiManager final
        {
        public:
            bool Initialize(Graphics::RenderDevice& rdevice, Graphics::DescriptorAllocator& da);
            void Shutdown();
            void Begin();
            void End();
            void Draw(Graphics::CommandContext& ctx);
            void SaveIni();
            void LoadIni();
        private:
            bool m_Initialized = false;
        };
    };
};

#endif // !NDEBUG
