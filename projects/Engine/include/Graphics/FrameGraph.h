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
    class DescriptorAllocator;
    class ResourceManager;
    class PipelineManager;
    class Renderer;
    class FrameGraph;
    class PassBuilder;
    class PassContext;

    struct GraphicsPipelineSettings;
    struct ComputePipelineSettings;
    struct DescriptorAllocator::TableID;

    /// @brief FrameGraph 上のリソースハンドル
    using ResourceHandle = uint32_t;

    /// @brief 無効なリソースハンドル値
    constexpr ResourceHandle InvalidResource = UINT32_MAX;

    /// @brief パスセットアップ関数
    using PassSetupFn = std::function<void(PassBuilder&)>;

    /// @brief パス実行関数
    using PassExecuteFn = std::function<void(PassContext&, CommandContext&)>;

    /// @brief PassID（インデックスのラッパ）
    struct PassID
    {
        uint32_t idx = 0;
    };

    /// @brief FrameGraph 上でのリソース用途
    enum class FGUsage : uint8_t
    {
        Texture,
        Buffer,
        RenderTarget,
        DepthStencil,
        UnorderedAccess,
        Unknown ///< 使用禁止
    };

    /// @brief パスから見たアクセス種別
    enum class FGAccess : uint8_t
    {
        Read,
        Write,
        ReadWrite,
        Unknown ///< 使用禁止
    };

    /// @brief FrameGraph 上で管理するリソース状態
    enum class FGState : uint8_t
    {
        Common,
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

    /// @brief 使用する GPU キュー種別
    enum class FGQueue : uint8_t
    {
        Graphics,
        Compute,
        Copy,
        Unknown ///< 使用禁止
    };

    /// @brief FrameGraph 用リソース記述
    struct ResourceDesc final
    {
        uint32_t width = 0;
        uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        FGUsage usage = FGUsage::Texture;
        std::optional<GlobalBufferType> existsGlobalBufferType = std::nullopt;
    };

    /// @brief ResourceDesc + Access から、パス中で使う FGState を決定する
    FGState DecideState(const ResourceDesc& desc, FGAccess access);

    /// @brief FGState を D3D12_RESOURCE_STATES に変換
    D3D12_RESOURCE_STATES FGStateToD3D12State(FGState state);

    /// @brief パスから見たリソース使用
    struct ResourceUse final
    {
        ResourceHandle handle;
        FGAccess access;
    };

    /// @brief バリアの種類
    enum class BarrierType : uint8_t
    {
        Transition,
        UAV,
    };

    /// @brief バリア情報（FrameGraph → Renderer.ApplyBarriers に渡す）
    struct BarrierInfo final
    {
        ResourceHandle handle;
        BarrierType type = BarrierType::Transition;
        FGState beforeState;
        FGState afterState;
    };

    /// @brief 1つのパスノード
    struct PassNode final
    {
        /// @brief パス名
        std::string name;

        /// @brief パスインデックス（m_Passes 内の index）
        uint32_t index = 0;

        /// @brief 使用するキュー種別
        FGQueue queue = FGQueue::Graphics;

        /// @brief このパスで読み取るリソース
        std::vector<ResourceUse> reads;

        /// @brief このパスで書き込むリソース
        std::vector<ResourceUse> writes;

        /// @brief 依存先パスのインデックスリスト（このパスから出るエッジ）
        std::vector<uint32_t> outEdges;

        /// @brief 依存元パスのインデックスリスト（このパスに入るエッジ）
        std::vector<uint32_t> inEdges;

        /// @brief パス実行関数
        PassExecuteFn executeFn;
    };

    /// @brief FrameGraph 上の仮想リソース情報
    struct VirtualResource final
    {
        /// @brief リソース名（デバッグ用）
        std::string  name;

        /// @brief リソース記述
        ResourceDesc desc;

        /// @brief 使用されるパスのインデックスリスト
        std::vector<uint32_t> usedInPass;

        /// @brief 使用開始パスインデックス
        uint32_t firstPass = UINT32_MAX;

        /// @brief 使用終了パスインデックス
        uint32_t lastPass = 0;

        /// @brief ResourceManager 上の物理リソース ID
        uint32_t physicalId = UINT32_MAX;

        /// @brief 描画用ディスクリプタテーブル ID（RTV/SRV 等）
        DescriptorAllocator::TableID rtvTableId;
        DescriptorAllocator::TableID dsvTableId;
        DescriptorAllocator::TableID srvTableId;

        /// @brief フレームの外での「休み状態」＝パス間を跨ぐときに戻す状態
        FGState initialState = FGState::Common;
    };

    /// @brief パス登録時に使うビルダー
    class PassBuilder final
    {
    public:
        /// @brief コンストラクタ
        PassBuilder(FrameGraph& fg, uint32_t passIndex)
            : m_FrameGraph(fg), m_PassIndex(passIndex)
        {
        }

        /// @brief 仮想リソース生成（初期ステート指定）
        /// @param name リソース名
        /// @param desc リソース記述
        /// @param initialState パスの外での休み状態
        ResourceHandle Create(std::string_view name, const ResourceDesc& desc, FGState initialState = FGState::Common);

        /// @brief 既存リソースを Read として使う（名前指定）
        ResourceHandle Read(std::string_view name);

        /// @brief 既存リソースを Write として使う（名前指定）
        ResourceHandle Write(std::string_view name);

        /// @brief 既存リソースを ReadWrite として使う（名前指定）
        ResourceHandle ReadWrite(std::string_view name);

        /// @brief 既存リソースを Read として使う（ハンドル指定）
        void Read(ResourceHandle h);

        /// @brief 既存リソースを Write として使う（ハンドル指定）
        void Write(ResourceHandle h);

        /// @brief 既存リソースを ReadWrite として使う（ハンドル指定）
        void ReadWrite(ResourceHandle h);

    private:
        FrameGraph& m_FrameGraph;
        uint32_t    m_PassIndex;

        ResourceHandle registerNewResource(std::string_view name, const ResourceDesc& desc, FGState initialState);
        ResourceHandle registerUse(std::string_view name, FGAccess access);
        void           registerUse(ResourceHandle h, FGAccess access);
    };

    /// @brief パス実行時のコンテキスト
    class PassContext final
    {
        friend class FrameGraph;
    public:
        /// @brief コンストラクタ
        PassContext(FrameGraph& fg,
            DescriptorAllocator& da,
            ResourceManager& rm,
            PipelineManager& pm,
            uint32_t passIndex)
            : m_FrameGraph(fg), m_DescriptorAllocator(da), m_ResourceManager(rm), m_PipelineManager(pm), m_PassIndex(passIndex)
        {
        }

        /// @brief パスインデックス取得
        uint32_t GetPassIndex() const noexcept { return m_PassIndex; }

        /// @brief グラフィックスパイプライン取得
        GraphicsPipelineSettings* GetGraphicsPipelineByName(const std::string& name);

        /// @brief コンピュートパイプライン取得
        ComputePipelineSettings* GetComputePipelineByName(const std::string& name);

    private:
        FrameGraph& m_FrameGraph;
        DescriptorAllocator& m_DescriptorAllocator;
        ResourceManager& m_ResourceManager;
        PipelineManager& m_PipelineManager;
        uint32_t             m_PassIndex;
    };

    /// @brief FrameGraph 本体
    class FrameGraph final
    {
    public:
        FrameGraph() = default;
        ~FrameGraph() = default;

        /// @brief デフォルトパスの生成（FinalColor など）
        void CreateDefaultPasses();

        /// @brief パス追加（キュー指定版）
        PassID AddPass(std::string_view name, FGQueue queue, PassSetupFn setupFn, PassExecuteFn executeFn);

        /// @brief パス依存・トポロジカルソート・物理リソース確保
        void Compile(DescriptorAllocator& da, ResourceManager& rm);

        /// @brief PassExecute の実行（実行時にバリア計算）
        void Execute(Renderer& renderer, DescriptorAllocator& da, ResourceManager& rm, PipelineManager& pm);

        /// @brief グラフのクリア
        void Clear()
        {
            m_Passes.clear();
            m_VResources.clear();
            m_SortedPasses.clear();
            m_NameToResource.clear();
            m_CurrentStates.clear();
        }

        /// @brief 仮想リソースの生成
        ResourceHandle CreateVirtualResource(std::string_view name, const ResourceDesc& desc, FGState initialState);

        /// @brief 仮想リソース取得
        const VirtualResource& GetVirtualResource(ResourceHandle h) const
        {
            return m_VResources[h];
        }

        /// @brief パス取得
        PassNode& GetPass(uint32_t passIndex)
        {
            return m_Passes[passIndex];
        }

        /// @brief 名前からリソースハンドルを検索
        ResourceHandle FindResourceHandle(std::string_view name) const
        {
            if (name.empty()) { return InvalidResource; }
            if (m_NameToResource.contains(std::string(name)))
            {
                return m_NameToResource.at(std::string(name));
            }
            else
            {
                return InvalidResource;
            }
        }

        /// @brief パス間依存関係の追加（before → after）
        void AddDependency(PassID before, PassID after);

    private:
        /// @brief 実行前バリアの適用（初期ステート → パス用ステート）
        void ApplyPassBarriersBegin(Renderer& renderer, CommandContext* cmd, const PassNode& pass);

        /// @brief 実行後バリアの適用（パス用ステート → 初期ステート）
        void ApplyPassBarriersEnd(Renderer& renderer, CommandContext* cmd, const PassNode& pass);

    private:
        /// @brief パス群
        std::vector<PassNode>        m_Passes;

        /// @brief 仮想リソース群
        std::vector<VirtualResource> m_VResources;

        /// @brief トポロジカルソート済みパスインデックス
        std::vector<uint32_t>        m_SortedPasses;

        /// @brief 名前 → リソースハンドル
        std::unordered_map<std::string, ResourceHandle> m_NameToResource;

        /// @brief 物理リソース（=仮想リソース）ごとの現在ステート
        /// 実行時にのみ更新し、Compile では触らない。
        std::vector<FGState> m_CurrentStates;
    };
}
