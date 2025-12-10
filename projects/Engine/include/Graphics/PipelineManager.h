#pragma once
#include <d3d12.h>
#include <d3d12shader.h>
#include <wrl.h>
#include <string>
#include <memory>
#include <array>
#include <unordered_map>
#include "include/Utility/FVector.h"
#include "include/Graphics/GpuBuffer.h"
#include "include/Graphics/DescriptorAllocator.h"

namespace Theatria::Graphics
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    class ResourceManager;
    class DescriptorAllocator;
    class ShaderCompiler;

    struct NumThreads
    {
        uint32_t x = 1;
        uint32_t y = 1;
        uint32_t z = 1;
    };

    enum class IndirectCommandType : uint8_t
    {
        Basic,
    };

    struct RBasicIndirectCommand
    {
        uint32_t ObjectId;
        uint32_t _pad[3]; // 16バイトアライメント用パディング

        // DrawIndexed
        D3D12_DRAW_INDEXED_ARGUMENTS DrawArgs;

        uint32_t _pad2[3]; // 16バイトアライメント用パディング
    };
    static_assert(sizeof(RBasicIndirectCommand) % 4 == 0, "RBasicIndirectCommand size must be multiple of 4 bytes.");

    enum class BlendMode : uint8_t
    {
        None,
        Normal,
        Add,
        Subtract,
        Multiply,
        Screen,
        kCount ///< BlendModeの数
    };

    struct GraphicsPipelineSettings
    {
        std::string name = "";///< Pipeline name
        // D3D12 Objects
        ComPtr<ID3D12RootSignature> rootSignature = nullptr;
        std::array<ComPtr<ID3D12PipelineState>, static_cast<size_t>(BlendMode::kCount)> pso;
        ComPtr<ID3D12CommandSignature> commandSignature = nullptr;
        // Indirect Args Buffer
        UINT indirectCommandCount = 256;
        RWStructuredBuffer<RBasicIndirectCommand> argsBuffer;
        DescriptorAllocator::TableID argsDescriptorTableID{};
        // Shader names
        std::string vs = "";///< Vertex Shader
        std::string ps = "";///< Pixel Shader
        std::string gs = "";///< Geometry Shader
        std::string hs = "";///< Hull Shader
        std::string ds = "";///< Domain Shader
    };

    struct ComputePipelineSettings
    {
        std::string name = "";///< Pipeline name
        // D3D12 Objects
        ComPtr<ID3D12RootSignature> rootSignature = nullptr;
        ComPtr<ID3D12PipelineState> pso = nullptr;
        ComPtr<ID3D12CommandSignature> commandSignature = nullptr;
        std::string cs = "";///< Compute Shader
        NumThreads numThreads = { 1, 1, 1 };///< Number of threads
    };

    struct MeshPipelineSettings
    {
        std::string name = "";///< Pipeline name
        // D3D12 Objects
        ComPtr<ID3D12RootSignature> rootSignature = nullptr;
        ComPtr<ID3D12PipelineState> pso = nullptr;
        ComPtr<ID3D12CommandSignature> commandSignature = nullptr;
        // Mesh Shader names
        std::string ms = "";///< Mesh Shader
        std::string as = "";///< Amplification Shader
        NumThreads numThreads = { 1, 1, 1 };///< Number of threads
    };

    class PipelineManager final
    {
    public:
        PipelineManager() = default;
        ~PipelineManager() = default;

        [[nodiscard]] bool Initialize(ID3D12Device* device, ShaderCompiler* compiler, DescriptorAllocator* descriptorAllocator);

        void CreateDefaultPipelines(ID3D12Device* device, ShaderCompiler* compiler);

        GraphicsPipelineSettings* GetGraphicsPipelineByName(const std::string& name)
        {
            if (m_GraphicsPipelineNameToIndex.contains(name))
            {
                return &m_GraphicsPipelines[m_GraphicsPipelineNameToIndex[name]];
            }
            return nullptr;
        }

        ComputePipelineSettings* GetComputePipelineByName(const std::string& name)
        {
            if (m_ComputePipelineNameToIndex.contains(name))
            {
                return &m_ComputePipelines[m_ComputePipelineNameToIndex[name]];
            }
            return nullptr;
        }
    private:
        void CreateGraphicsPipeline(ID3D12Device* device, GraphicsPipelineSettings& setting, ShaderCompiler* compiler);
        void GetReflectionRootParms(ID3D12ShaderReflection* shaderRef,
            D3D12_SHADER_DESC shaderDesc,
            D3D12_SHADER_VISIBILITY shaderVis,
            std::vector<D3D12_ROOT_PARAMETER>& outRootParms,
            std::vector<D3D12_STATIC_SAMPLER_DESC>& outStaticSamplers,
            std::vector<D3D12_DESCRIPTOR_RANGE>& outRenges,
            D3D12_DESCRIPTOR_RANGE& outTexRenge,
            bool& outUseTexBuf);

        DescriptorAllocator* m_pDescriptorAllocator = nullptr;

        Utility::FVector<GraphicsPipelineSettings> m_GraphicsPipelines;
        std::unordered_map<std::string, uint32_t> m_GraphicsPipelineNameToIndex;
        Utility::FVector<ComputePipelineSettings> m_ComputePipelines;
        std::unordered_map<std::string, uint32_t> m_ComputePipelineNameToIndex;
        Utility::FVector<MeshPipelineSettings> m_MeshPipelines;
        std::unordered_map<std::string, uint32_t> m_MeshPipelineNameToIndex;
    };
}

