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
    class ResourceManager;
    class Renderer;
    class FrameGraph;

    class PassBuilder;
    class PassContext;

    using ResourceHandle = uint32_t;
    constexpr ResourceHandle InvalidResource = UINT32_MAX;
    using PassSetupFn = std::function<void(PassBuilder&)>;
    using PassExecuteFn = std::function<void(PassContext&, GraphicsCommandContext&)>;

    struct PassID
    {
        uint32_t idx = 0;
    };

    enum class FGUsage : uint8_t
    {
        Texture,
        Buffer,
        RenderTarget,
        DepthStencil,
        UnorderedAccess,
        kindCount ///< 使用禁止
    };

    enum class FGAccess : uint8_t
    {
        Read,
        Write,
        ReadWrite
    };

    enum class FGState : uint8_t
    {
        RenderTarget,
        DepthWrite,
        DepthRead,
        ShaderRead,
        UnorderedAccess,
        CopySrc,
        CopyDst,
        Present,
        Unknown ///< 使用禁止
    };

    struct ResourceDesc final
    {
        uint32_t width = 0;
        uint32_t height = 0;
        FGUsage usage = FGUsage::Texture;
    };

    FGState DecideState(const ResourceDesc& desc, FGAccess access);

    struct ResourceUse final
    {
        ResourceHandle handle;
        FGAccess access;
    };

    struct BarrierInfo final
    {
        ResourceHandle handle;
        FGState beforeState;
        FGState afterState;
    };

    struct PassNode final
    {
        std::string name;
        uint32_t index = 0;

        std::vector<ResourceUse> reads;
        std::vector<ResourceUse> writes;

        std::vector<uint32_t> outEdges;
        std::vector<uint32_t> inEdges;

        std::vector<BarrierInfo> barriers; // パスの冒頭のバリア

        PassExecuteFn executeFn;
    };

    struct VirtualResource final
    {
        std::string  name;
        ResourceDesc desc;

        // 使用されるパスの index のリスト（build 時に埋める）
        std::vector<uint32_t> usedInPass;

        // 寿命 [firstPass, lastPass]
        uint32_t firstPass = UINT32_MAX;
        uint32_t lastPass = 0;

        // Compile 後に物理リソースIDを持つ（ResourceManager用）
        uint32_t physicalId = UINT32_MAX;
    };

    class PassBuilder final
    {
    public:
        PassBuilder(FrameGraph& fg, uint32_t passIndex)
            : m_FrameGraph(fg), m_PassIndex(passIndex)
        {
        }

        // ---- 仮想リソース生成 ----
        ResourceHandle Create(const char* name, const ResourceDesc& desc)
        {
            return registerNewResource(name, desc);
        }

        // ---- Read/Write 宣言（名前版）----
        ResourceHandle Read(const char* name) { return registerUse(name, FGAccess::Read); }
        ResourceHandle Write(const char* name) { return registerUse(name, FGAccess::Write); }
        ResourceHandle ReadWrite(const char* name) { return registerUse(name, FGAccess::ReadWrite); }

        // ---- Read/Write 宣言（Handle版）----
        void Read(ResourceHandle h) { registerUse(h, FGAccess::Read); }
        void Write(ResourceHandle h) { registerUse(h, FGAccess::Write); }
        void ReadWrite(ResourceHandle h) { registerUse(h, FGAccess::ReadWrite); }

        // any other setup functions color,depth, etc.
    private:
        FrameGraph& m_FrameGraph;
        uint32_t    m_PassIndex;

        ResourceHandle registerNewResource(const char* name, const ResourceDesc& desc);
        ResourceHandle registerUse(const char* name, FGAccess access);
        void           registerUse(ResourceHandle h, FGAccess access);
    };

    class PassContext final
    {
    public:
        PassContext(FrameGraph& fg,
            ResourceManager& rm,
            uint32_t passIndex)
            : m_FrameGraph(fg), m_ResourceManager(rm), m_PassIndex(passIndex)
        {
        }

        uint32_t GetPassIndex() const noexcept { return m_PassIndex; }

        //// ---- リソース情報 ----
        //const ResourceDesc& GetResourceDesc(ResourceHandle h) const;
        //// 生リソース（GPUリソース）
        //ID3D12Resource* GetResource(ResourceHandle h) const;

        //// ---- ビュー取得 ----
        //D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(ResourceHandle h) const;
        //D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(ResourceHandle h) const;
        //D3D12_GPU_DESCRIPTOR_HANDLE GetSRV(ResourceHandle h) const;
        //D3D12_GPU_DESCRIPTOR_HANDLE GetUAV(ResourceHandle h) const;

        //// 名前から取る版（便利だけど遅いのでデバッグ用）
        //ResourceHandle Find(const char* name) const;
        //D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(const char* name) const
        //{
        //    return GetRTV(Find(name));
        //}

        //// ビューポート/シザーなどを frameGraph 側で決めて渡したいならここでもいい
        //D3D12_VIEWPORT GetViewport() const;
        //D3D12_RECT     GetScissor()  const;

    private:
        FrameGraph& m_FrameGraph;
        ResourceManager& m_ResourceManager;
        uint32_t         m_PassIndex;
    };

    class FrameGraph final
    {
    public:
        FrameGraph() = default;
        ~FrameGraph() = default;

        /// @brief パスの追加
        PassID AddPass(const std::string& name, PassSetupFn setupFn, PassExecuteFn executeFn);

        /// @brief バリア、順序付け
        void Compile(ResourceManager& rm);
        /// @brief PassExecuteの実行
        void Execute(Renderer& renderer, ResourceManager& rm);
        /// @brief クリア
        void Clear()
        {
            m_Passes.clear();
            m_VResources.clear();
            m_SortedPasses.clear();
            m_NameToResource.clear();
        }
        ResourceHandle CreateVirtualResource(const char* name, const ResourceDesc& desc)
        {
            // すでに同名があったらどうするかはポリシー次第。
            // ここでは「新規作成」しか許さない。
            if (name && m_NameToResource.find(name) != m_NameToResource.end())
            {
                throw std::runtime_error(std::string("FrameGraph: resource already exists '") + name + "'");
            }

            ResourceHandle h = static_cast<ResourceHandle>(m_VResources.size());
            VirtualResource vr;
            vr.name = name ? name : "";
            vr.desc = desc;
            m_VResources.push_back(std::move(vr));

            if (name && *name)
            {
                m_NameToResource.emplace(vr.name, h);
            }
            return h;
        }
        const VirtualResource& GetVirtualResource(ResourceHandle h) const
        {
            return m_VResources[h];
        }
        PassNode& GetPass(uint32_t passIndex)
        {
            return m_Passes[passIndex];
        }
        ResourceHandle FindResourceHandle(const char* name) const
        {
            if (!name) return InvalidResource;
            auto it = m_NameToResource.find(name);
            if (it == m_NameToResource.end()) return InvalidResource;
            return it->second;
        }
    private:
        std::vector<PassNode>       m_Passes;      // AddPass で増える
        std::vector<VirtualResource> m_VResources;  // Builder::CreateResource などで増える

        std::vector<uint32_t> m_SortedPasses;      // トポロジカルソート結果

        // 名前→ResourceHandle
        std::unordered_map<std::string, ResourceHandle> m_NameToResource;
    };
};
