#include "pch.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/Renderer.h"
#include "config/engineConfig.h"
#include "include/Core/LogAssert.h"

bool Theatria::Graphics::ResourceManager::Initialize(RenderDevice* device, DescriptorAllocator* descAllocator, Renderer* renderer)
{
    m_pDevice = device;
    m_pDescAllocator = descAllocator;
    m_pRenderer = renderer;
    CreateGlobalBuffers();

    return true;
}

void Theatria::Graphics::ResourceManager::CreateGlobalBuffers()
{
#ifndef NDEBUG
    std::lock_guard debuglock(m_DebugVPMutex);
#endif // !NDEBUG

    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        m_GlobalObjectBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 1024);
        m_GlobalTransformBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 1024);
        m_GlobalModelInfoBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 256);
#ifndef NDEBUG
        m_DebugVP[i].CreateBuffer(m_pDevice->GetDevice());
#endif
    }
#ifndef NDEBUG
    m_DebugVPUploadBuffer.CreateBuffer(m_pDevice->GetDevice(), 1);
#endif
}

[[nodiscard]]
uint32_t Theatria::Graphics::ResourceManager::CreateVertexBuffer(uint32_t numVertices, const std::vector<Assets::VertexData>& vec)
{
    std::lock_guard<std::mutex> lock(m_VertexBufferMutex);
    VertexBuffer<Assets::VertexData> buffer;
    buffer.CreateBuffer(m_pDevice->GetDevice(), numVertices);
    UploadBuffer<Assets::VertexData> uploadBuffer;
    uploadBuffer.CreateBuffer(m_pDevice->GetDevice(), numVertices);
    // データコピー、アップロード
    std::span<Assets::VertexData> mappedData = uploadBuffer.GetMappedData();
    memcpy(mappedData.data(), vec.data(), sizeof(Assets::VertexData) * numVertices);
    // コマンドリストでコピー
    auto queue = m_pDevice->m_QueuePool->GetComputeQueue();
    auto cmd = m_pRenderer->BeginComputePass();
    // バリア挿入
    cmd->BarrierTransition(
        &buffer,
        buffer.GetUseState(),
        D3D12_RESOURCE_STATE_COPY_DEST);// コピー先へ
    // コピー
    cmd->GetCommandList()->CopyResource(
        buffer.GetResource(),
        uploadBuffer.GetResource());
    // バリア挿入
    cmd->BarrierTransition(
        &buffer,
        buffer.GetUseState(),
        D3D12_RESOURCE_STATE_COMMON);
    m_pRenderer->EndComputePass(cmd);
    m_pDevice->m_QueuePool->ReturnQueue(queue);
    uploadBuffer.Destroy();// アップロードバッファ破棄
    uint32_t idx = static_cast<uint32_t>(m_VertexBuffers.emplace_back(std::move(buffer)));
    return idx;
}

[[nodiscard]]
uint32_t Theatria::Graphics::ResourceManager::CreateIndexBuffer(uint32_t numIndices, const std::vector<uint32_t>& vec)
{
    std::lock_guard<std::mutex> lock(m_IndexBufferMutex);
    IndexBuffer<uint32_t> buffer;
    buffer.CreateBuffer(m_pDevice->GetDevice(), numIndices);
    UploadBuffer<uint32_t> uploadBuffer;
    uploadBuffer.CreateBuffer(m_pDevice->GetDevice(), numIndices);
    // データコピー、アップロード
    std::span<uint32_t> mappedData = uploadBuffer.GetMappedData();
    memcpy(mappedData.data(), vec.data(), sizeof(uint32_t) * numIndices);
    // コマンドリストでコピー
    auto queue = m_pDevice->m_QueuePool->GetComputeQueue();
    auto cmd = m_pRenderer->BeginComputePass();
    // バリア挿入
    cmd->BarrierTransition(
        &buffer,
        buffer.GetUseState(),
        D3D12_RESOURCE_STATE_COPY_DEST);// コピー先へ
    // コピー
    cmd->GetCommandList()->CopyResource(
        buffer.GetResource(),
        uploadBuffer.GetResource());
    // バリア挿入
    cmd->BarrierTransition(
        &buffer,
        buffer.GetUseState(),
        D3D12_RESOURCE_STATE_COMMON);
    m_pRenderer->EndComputePass(cmd);
    m_pDevice->m_QueuePool->ReturnQueue(queue);
    uploadBuffer.Destroy();// アップロードバッファ破棄
    uint32_t idx = static_cast<uint32_t>(m_IndexBuffers.emplace_back(std::move(buffer)));
    return idx;
}
