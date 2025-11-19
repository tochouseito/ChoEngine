#include "pch.h"
#include "include/Graphics/GPUCommand.h"
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
