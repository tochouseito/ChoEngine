#pragma once
#ifndef NDEBUG
#include <string>

namespace Theatria::Editor
{
    class BaseEditor
    {
    public:
        virtual ~BaseEditor() = default;
        virtual void Initialize() = 0;
        virtual void Update() = 0;
    };
}

#endif // !NDEBUG
