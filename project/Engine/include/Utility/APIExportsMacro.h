#pragma once
#ifdef ENGINE_EXPORTS
#define THEATRIA_API __declspec(dllexport)
#else
#define THEATRIA_API __declspec(dllimport)
#endif
class THEATRIA_API DummyClass final
{
public:
    DummyClass() = default;
    ~DummyClass() = default;
private:
    void func()
    {
        v++;
    }
    int v = 0;
};
