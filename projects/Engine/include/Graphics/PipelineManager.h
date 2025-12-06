#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <string>
#include "include/Utility/FVector.h"

namespace Theatria::Graphics
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    class ShaderCompiler;

    struct Pipeline
    {
        std::string name = "";///< Pipeline name
        // D3D12 Objects
        ComPtr<ID3D12RootSignature> rootSignature = nullptr;
        ComPtr<ID3D12PipelineState> pso = nullptr;
    };

    struct GraphicsPipelineSettings : public Pipeline
    {
        // Shader names
        std::string vs = "";///< Vertex Shader
        std::string ps = "";///< Pixel Shader
        std::string gs = "";///< Geometry Shader
        std::string hs = "";///< Hull Shader
        std::string ds = "";///< Domain Shader
    };

    struct ComputePipelineSettings : public Pipeline
    {
        std::string cs = "";///< Compute Shader
    };

    struct MeshPipelineSettings : public Pipeline
    {
        // Mesh Shader names
        std::string ms = "";///< Mesh Shader
        std::string as = "";///< Amplification Shader
    };

    class PipelineManager final
    {
    public:
        PipelineManager() = default;
        ~PipelineManager() = default;

        void CreateDefaultPipelines(ID3D12Device* device, ShaderCompiler* compiler);
    private:
        D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPipelineDesc()
        {

        }

        Utility::FVector<GraphicsPipelineSettings> m_GraphicsPipelines;
        Utility::FVector<ComputePipelineSettings> m_ComputePipelines;
        Utility::FVector<MeshPipelineSettings> m_MeshPipelines;
    };
}

