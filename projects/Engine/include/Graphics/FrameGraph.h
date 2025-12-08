#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <optional>
#include <unordered_map>

#include "include/Graphics/GPUCommand.h"
#include "include/Graphics/GlobalBuffer.h"

namespace Theatria::Graphics
{
    class ResourceManager;
    class PipelineManager;
    class Renderer;
    class FrameGraph;
    class PassBuilder;
    class PassContext;

    using ResourceHandle = uint32_t;
    constexpr ResourceHandle InvalidResource = UINT32_MAX;
    using PassSetupFn = std::function<void(PassBuilder&)>;
    using PassExecuteFn = std::function<void(PassContext&, CommandContext&)>;

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
        Unknown ///< 使用禁止
    };

    enum class FGAccess : uint8_t
    {
        Read,
        Write,
        ReadWrite,
        Unknown ///< 使用禁止
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

    enum class FGQueue : uint8_t
    {
        Graphics,
        Compute,
        Copy,
        Unknown ///< 使用禁止
    };

    struct ResourceDesc final
    {
        uint32_t width = 0;
        uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        FGUsage usage = FGUsage::Texture;
        std::optional<GlobalBufferType> existsGlobalBufferType = std::nullopt;
    };

    FGState DecideState(const ResourceDesc& desc, FGAccess access);
    D3D12_RESOURCE_STATES FGStateToD3D12State(FGState state);

    struct ResourceUse final
    {
        ResourceHandle handle;
        FGAccess access;
    };

    enum class BarrierType : uint8_t
    {
        Transition,
        UAV,
    };

    struct BarrierInfo final
    {
        ResourceHandle handle;
        BarrierType type = BarrierType::Transition;
        FGState beforeState;
        FGState afterState;
    };

    struct PassNode final
    {
        std::string name; ///< パス名
        uint32_t index = 0; ///< パスのインデックス

        FGQueue queue = FGQueue::Graphics; ///< 使用するキュー

        std::vector<ResourceUse> reads; ///< 読み取りリソース
        std::vector<ResourceUse> writes;///< 書き込みリソース

        std::vector<uint32_t> outEdges;///< 出力先パスのインデックスリスト
        std::vector<uint32_t> inEdges; ///< 入力元パスのインデックスリスト

        std::vector<BarrierInfo> barriers; // パスの冒頭のバリア

        PassExecuteFn executeFn; ///< 実行関数
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
        FGState initialState = FGState::Unknown;
    };

    class PassBuilder final
    {
    public:
        PassBuilder(FrameGraph& fg, uint32_t passIndex)
            : m_FrameGraph(fg), m_PassIndex(passIndex)
        {
        }

        // ---- 仮想リソース生成 ----
        ResourceHandle Create(std::string_view name, const ResourceDesc& desc)
        {
            return registerNewResource(name, desc);
        }

        // ---- Read/Write 宣言（名前版）----
        ResourceHandle Read(std::string_view name) { return registerUse(name, FGAccess::Read); }
        ResourceHandle Write(std::string_view name) { return registerUse(name, FGAccess::Write); }
        ResourceHandle ReadWrite(std::string_view name) { return registerUse(name, FGAccess::ReadWrite); }

        // ---- Read/Write 宣言（Handle版）----
        void Read(ResourceHandle h) { registerUse(h, FGAccess::Read); }
        void Write(ResourceHandle h) { registerUse(h, FGAccess::Write); }
        void ReadWrite(ResourceHandle h) { registerUse(h, FGAccess::ReadWrite); }

        // any other setup functions color,depth, etc.
    private:
        FrameGraph& m_FrameGraph;
        uint32_t    m_PassIndex;

        ResourceHandle registerNewResource(std::string_view name, const ResourceDesc& desc);
        ResourceHandle registerUse(std::string_view name, FGAccess access);
        void           registerUse(ResourceHandle h, FGAccess access);
    };

    class PassContext final
    {
    public:
        PassContext(FrameGraph& fg,
            ResourceManager& rm,
            PipelineManager& pm,
            uint32_t passIndex)
            : m_FrameGraph(fg), m_ResourceManager(rm), m_PipelineManager(pm), m_PassIndex(passIndex)
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
        //ResourceHandle Find(std::string_view name) const;
        //D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(std::string_view name) const
        //{
        //    return GetRTV(Find(name));
        //}

        //// ビューポート/シザーなどを frameGraph 側で決めて渡したいならここでもいい
        //D3D12_VIEWPORT GetViewport() const;
        //D3D12_RECT     GetScissor()  const;

    private:
        FrameGraph& m_FrameGraph;
        ResourceManager& m_ResourceManager;
        PipelineManager& m_PipelineManager;
        uint32_t         m_PassIndex;
    };

    class FrameGraph final
    {
    public:
        FrameGraph() = default;
        ~FrameGraph() = default;

        void CreateDefaultPasses();

        /// @brief パスの追加(任意のキュー指定版)
        PassID AddPass(std::string_view name, FGQueue queue, PassSetupFn setupFn, PassExecuteFn executeFn);

        /// @brief バリア、順序付け
        void Compile(ResourceManager& rm);
        /// @brief PassExecuteの実行
        void Execute(Renderer& renderer, ResourceManager& rm, PipelineManager& pm);
        /// @brief クリア
        void Clear()
        {
            m_Passes.clear();
            m_VResources.clear();
            m_SortedPasses.clear();
            m_NameToResource.clear();
        }
        ResourceHandle CreateVirtualResource(std::string_view name, const ResourceDesc& desc);
        const VirtualResource& GetVirtualResource(ResourceHandle h) const
        {
            return m_VResources[h];
        }
        PassNode& GetPass(uint32_t passIndex)
        {
            return m_Passes[passIndex];
        }
        ResourceHandle FindResourceHandle(std::string_view name) const
        {
            if (name.empty()) { return InvalidResource; }
            if (m_NameToResource.contains(name.data()))
            {
                return m_NameToResource.at(name.data());
            }
            else
            {
                return InvalidResource;
            }
        }
        /// @brief 依存関係追加
        /// @param before 
        /// @param after 
        void AddDependency(PassID before, PassID after);

    private:
        std::vector<PassNode>       m_Passes;      // AddPass で増える
        std::vector<VirtualResource> m_VResources;  // Builder::CreateResource などで増える

        std::vector<uint32_t> m_SortedPasses;      // トポロジカルソート結果

        // 名前→ResourceHandle
        std::unordered_map<std::string, ResourceHandle> m_NameToResource;
    };
};
