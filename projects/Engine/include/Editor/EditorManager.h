#pragma once
#ifndef NDEBUG
namespace Theatria::Editor
{
    class EditorManager final
    {
    public:
        bool Initialize();
        void Shutdown();
        void Update();

    private:
        void BackDockingWindows();
    };
}
#endif // !NDEBUG

