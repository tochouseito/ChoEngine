#include "pch.h"
#include "include/Graphics/GPUCommand.h"
#include "config/engineConfig.h"
#include "include/Core/LogAssert.h"
#include "include/Graphics/ResourceLeakChecker.h"

/// @brief コンストラクタ
Theatria::Graphics::CommandContext::CommandContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    // コマンドアロケータを生成する
    if (!Core::LogAssert::Verify(
        device->CreateCommandAllocator(
            type,
            IID_PPV_ARGS(&m_Allocator)
        ),
        "RenderDevice",
        "Failed to create CommandAllocator."))
    {
        Core::LogAssert::Check(false, "RenderDevice", "CommandAllocator creation failed.");
    }
    SetD3D12Name(m_Allocator.Get());

    // コマンドリストを生成する
    if (!Core::LogAssert::Verify(
        device->CreateCommandList(
            0,
            type,
            m_Allocator.Get(),
            nullptr,
            IID_PPV_ARGS(&m_List)
        ),
        "RenderDevice",
        "Failed to create CommandList."))
    {
        Core::LogAssert::Check(false, "RenderDevice", "CommandList creation failed.");
    }
    SetD3D12Name(m_List.Get());

    m_List->Close();  // 初期状態で閉じておく
}

void Theatria::Graphics::CommandContext::Reset()
{
    // コマンドアロケータをリセットする
    HRESULT hr = m_Allocator->Reset();
    Core::LogAssert::Check(
        hr,
        "RenderDevice",
        "Failed to reset CommandAllocator.");
    // コマンドリストをリセットする
    hr = m_List->Reset(m_Allocator.Get(), nullptr);
    Core::LogAssert::Check(
        hr,
        "RenderDevice",
        "Failed to reset CommandList.");
}

void Theatria::Graphics::CommandContext::Close()
{
    // コマンドリストを閉じる
    HRESULT hr = m_List->Close();
    Core::LogAssert::Check(
        hr,
        "RenderDevice",
        "Failed to close CommandList.");
}

void Theatria::Graphics::CommandContext::SetDescriptorHeap(ID3D12DescriptorHeap* pHeap)
{
    // ディスクリプタヒープを設定する
    ID3D12DescriptorHeap* heaps[] = { pHeap };
    // コマンドリストにディスクリプタヒープを設定する
    m_List->SetDescriptorHeaps(_countof(heaps), heaps);
}

void Theatria::Graphics::CommandContext::ResourceBarrier(UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers)
{
    // リソースバリアの設定
    m_List->ResourceBarrier(NumBarriers, pBarriers);
}

void Theatria::Graphics::CommandContext::BarrierTransition(GpuResource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
{
    // TransitionBarrierの設定
    D3D12_RESOURCE_BARRIER barrier{};
    // 今回のバリアはTransition
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // Noneにしておく
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    // バリアを張る対象のリソース。現在のバックバッファに対して行う
    barrier.Transition.pResource = pResource->GetResource();
    // 遷移前（現在）のResourceState
    barrier.Transition.StateBefore = Before;
    // 遷移後のResourceState
    barrier.Transition.StateAfter = After;
    // 全てのミップマップに対してバリアを張る
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    // TransitionBarrierを張る
    ResourceBarrier(1, &barrier);
    // リソースステートの更新
    pResource->SetUseState(After);
}

void Theatria::Graphics::CommandContext::BarrierUAV(GpuResource* pResource)
{
    // 並列処理の阻止
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = pResource->GetResource();
    // UAVバリアを張る
    ResourceBarrier(1, &barrier);
}

void Theatria::Graphics::CommandContext::SetViewport(D3D12_VIEWPORT viewport)
{
    m_List->RSSetViewports(1, &viewport);
}

void Theatria::Graphics::CommandContext::SetScissorRect(D3D12_RECT rect)
{
    m_List->RSSetScissorRects(1, &rect);
}

void Theatria::Graphics::CommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
    m_List->IASetPrimitiveTopology(topology);
}

void Theatria::Graphics::CommandContext::SetRenderTargets(UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors, BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor)
{
    m_List->OMSetRenderTargets(
        NumRenderTargetDescriptors,
        pRenderTargetDescriptors,
        RTsSingleHandleToDescriptorRange,
        pDepthStencilDescriptor);
}

void Theatria::Graphics::CommandContext::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT ColorRGBA[4], UINT NumRects, const D3D12_RECT* pRects)
{
    m_List->ClearRenderTargetView(
        RenderTargetView,
        ColorRGBA,
        NumRects,
        pRects);
}

void Theatria::Graphics::CommandContext::ClearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource* pResource, const UINT Values[4], UINT NumRects, const D3D12_RECT* pRects)
{
    m_List->ClearUnorderedAccessViewUint(
        ViewGPUHandleInCurrentHeap,
        ViewCPUHandle,
        pResource,
        Values,
        NumRects,
        pRects);
}

void Theatria::Graphics::CommandContext::SetPipelineState(ID3D12PipelineState* pPipelineState)
{
    m_List->SetPipelineState(pPipelineState);
}

void Theatria::Graphics::CommandContext::SetGraphicsRootSignature(ID3D12RootSignature* pRootSignature)
{
    m_List->SetGraphicsRootSignature(pRootSignature);
}

void Theatria::Graphics::CommandContext::SetComputeRootSignature(ID3D12RootSignature* pRootSignature)
{
    m_List->SetComputeRootSignature(pRootSignature);
}

void Theatria::Graphics::CommandContext::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    m_List->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

void Theatria::Graphics::CommandContext::SetComputeRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor)
{
    m_List->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

/// @brief コンストラクタ
Theatria::Graphics::QueueContext::QueueContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    // フェンスの作成
    m_Fence.Reset();
    m_FenceValue = 0;// 初期値0でFenceを作る
    if (!Core::LogAssert::Verify(
        device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)),
        "RenderDevice",
        "Failed to create fence."))
    {
        Core::LogAssert::Check(false, "RenderDevice", "Fence creation failed.");
    }
    SetD3D12Name(m_Fence.Get());
    m_FenceValue++; // 次に使う値をインクリメントしておく
    // FenceのSignalを持つためのイベントを作成する
    m_FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!Core::LogAssert::Verify(
        m_FenceEvent != nullptr,
        "RenderDevice",
        "Failed to create fence event."))
    {
        Core::LogAssert::Check(false, "RenderDevice", "Fence event creation failed.");
    }
    // コマンドキューの作成
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    if (!Core::LogAssert::Verify(
        SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue))),
        "RenderDevice",
        "Failed to create command queue."))
    {
        Core::LogAssert::Check(false, "RenderDevice", "Command queue creation failed.");
    }
    SetD3D12Name(m_CommandQueue.Get(), L"this Target");
}
