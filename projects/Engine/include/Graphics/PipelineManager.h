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

    class PipelineManager final
    {
    public:
        PipelineManager() = default;
        ~PipelineManager() = default;
    private:
        struct Pipeline final
        {
            std::string name = "";
            ComPtr<ID3D12RootSignature> rootSignature = nullptr;
            ComPtr<ID3D12PipelineState> pso = nullptr;
        };

        Utility::FVector<Pipeline> m_Pipelines;
    };
}

