#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <unordered_map>

#include "include/Graphics/GPUCommand.h"

namespace Theatria::Graphics
{
    enum class Usage : uint8_t
    {
        Texture,
        Buffer,
        RenderTarget,
        DepthStencil
    };

    struct ResourceDesc final
    {
        uint32_t width = 0;
        uint32_t height = 0;
        
    };

    enum class Access : uint8_t
    {
        Read,
        Write,
        ReadWrite
    };

    enum class State : uint8_t
    {
        RenderTarget,
        DepthWrite,
        DepthRead,
        ShaderRead,
        UnorderedAccess,
        CopySrc,
        CopyDst,
        Present,
        kindCount ///< 使用禁止
    };

    class PassBuilder final
    {
    public:
        PassBuilder() = default;
        ~PassBuilder() = default;

        uint32_t CreateResource(const std::string& name);

    };

    class PassContext final
    {
    public:

    };

    using PassSetupFn = std::function<void(PassBuilder&)>;
    using PassExecuteFn = std::function<void(PassContext&, CommandContext&)>;

    class FrameGraph final
    {
    public:
        struct PassID
        {
            uint32_t idx = 0;
        };

        FrameGraph() = default;
        ~FrameGraph() = default;

        PassID AddPass(const std::string& name, PassSetupFn setupFn, PassExecuteFn executeFn);

        void Compile();
        void Execute();

        void Clear();
    private:
        std::unordered_map<std::string, uint32_t> m_ResourceNameToIndex;
    };
};
