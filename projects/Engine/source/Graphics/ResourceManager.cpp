#include "pch.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/Renderer.h"
#include "config/engineConfig.h"
#include "include/Core/LogAssert.h"

#include <ChoMath/include/choMath.h>

bool Theatria::Graphics::ResourceManager::Initialize(RenderDevice* device, DescriptorAllocator* descAllocator, Renderer* renderer)
{
    m_pDevice = device;
    m_pDescAllocator = descAllocator;
    m_pRenderer = renderer;
    CreateGlobalBuffers();

    auto upObjBuf = m_GlobalObjectBuffer.GetUploadBuffer();
    auto upTransBuf = m_GlobalTransformBuffer.GetUploadBuffer();
    auto upModelInfoBuf = m_GlobalMeshInfoBuffer.GetUploadBuffer();
    uint32_t objIdx = m_GlobalObjectBuffer.Allocate();
    uint32_t transIdx = m_GlobalTransformBuffer.Allocate();
    std::span<ShaderStruct::SObject> objData = upObjBuf.GetMappedData();
    std::span<ShaderStruct::STransform> transData = upTransBuf.GetMappedData();
    ShaderStruct::SObject defaultObj{};
    defaultObj.id = objIdx;
    defaultObj.visible = true;
    defaultObj.meshId = 0;
    defaultObj.transformId = transIdx;
    objData[objIdx] = defaultObj;
    ShaderStruct::STransform defaultTrans{};
    defaultTrans.worldMatrix = Math::float4x4::Identity();
    transData[transIdx] = defaultTrans;

    return true;
}

void Theatria::Graphics::ResourceManager::CreateGlobalBuffers()
{
#ifndef NDEBUG
    std::lock_guard debuglock(m_DebugVPMutex);
    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        m_DebugVP[i].CreateBuffer(m_pDevice->GetDevice());
    }
    m_DebugVPUploadBuffer.CreateBuffer(m_pDevice->GetDevice(), 1);
    auto span = m_DebugVPUploadBuffer.GetMappedData();
    Math::float3 debugCamPos = { 0.0f, 0.0f, -5.0f };
    Math::float3 debugCamRot = { 0.0f, 0.0f, 0.0f };
    Math::float3 debugCamScale = { 1.0f, 1.0f, 1.0f };
    Math::float4x4 matW = Math::MakeAffineMatrix(debugCamScale, debugCamRot, debugCamPos);
    span[0].view = Math::float4x4::Inverse(matW);
    span[0].projection = Math::PerspectiveFovMatrix(45.0f * Math::PI / 180.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
#endif
    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        // SRVのテーブル順番が正しくなるように注意
        m_GlobalObjectBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 1024, i);
        m_GlobalTransformBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 1024, i);
        m_GlobalMeshInfoBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 256, i);
    }
    m_GlobalObjectBuffer.CreateUploadBuffer(m_pDevice->GetDevice(), 1024);
    m_GlobalTransformBuffer.CreateUploadBuffer(m_pDevice->GetDevice(), 1024);
    m_GlobalMeshInfoBuffer.CreateUploadBuffer(m_pDevice->GetDevice(), 256);
    m_IndirectCommandCountBuffer.CreateBuffer(m_pDevice->GetDevice(), 1);
    m_IndirectCommandCountBufferDescriptorIDs = m_pDescAllocator->Allocate(DescriptorAllocator::TableKind::Buffers);
    m_pDescAllocator->CreateUAVRawBuffer(m_IndirectCommandCountBufferDescriptorIDs, &m_IndirectCommandCountBuffer);
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

void Theatria::Graphics::ResourceManager::RemakeIntegratedVBIB(const std::vector<Assets::VertexData>& vertices, const std::vector<uint32_t>& indices)
{
    std::lock_guard<std::mutex> lock(m_IntVBIBMutex);
    // 頂点バッファ再作成

    // 1) 破棄
    m_IntegratedVertexBuffer.Destroy();
    m_IntegratedIndexBuffer.Destroy();
    // 2) 作成
    m_IntegratedVertexBuffer.CreateBuffer(m_pDevice->GetDevice(), static_cast<UINT>(vertices.size()));
    m_IntegratedIndexBuffer.CreateBuffer(m_pDevice->GetDevice(), static_cast<UINT>(indices.size()));
    // アップロードバッファ作成
    UploadBuffer<Assets::VertexData> vertexUploadBuffer;
    vertexUploadBuffer.CreateBuffer(m_pDevice->GetDevice(), static_cast<UINT>(vertices.size()));
    UploadBuffer<uint32_t> indexUploadBuffer;
    indexUploadBuffer.CreateBuffer(m_pDevice->GetDevice(), static_cast<UINT>(indices.size()));
    // データコピー、アップロード
    {
        std::span<Assets::VertexData> mappedData = vertexUploadBuffer.GetMappedData();
        memcpy(mappedData.data(), vertices.data(), sizeof(Assets::VertexData) * vertices.size());
    }
    {
        std::span<uint32_t> mappedData = indexUploadBuffer.GetMappedData();
        memcpy(mappedData.data(), indices.data(), sizeof(uint32_t) * indices.size());
    }
    // コマンドリストでコピー
    auto queue = m_pDevice->m_QueuePool->GetComputeQueue();
    auto cmd = m_pRenderer->BeginComputePass();
    // バリア挿入
    cmd->BarrierTransition(
        &m_IntegratedVertexBuffer,
        m_IntegratedVertexBuffer.GetUseState(),
        D3D12_RESOURCE_STATE_COPY_DEST);// コピー先へ
    cmd->BarrierTransition(
        &m_IntegratedIndexBuffer,
        m_IntegratedIndexBuffer.GetUseState(),
        D3D12_RESOURCE_STATE_COPY_DEST);// コピー先へ
    // コピー
    cmd->GetCommandList()->CopyResource(
        m_IntegratedVertexBuffer.GetResource(),
        vertexUploadBuffer.GetResource());
    cmd->GetCommandList()->CopyResource(
        m_IntegratedIndexBuffer.GetResource(),
        indexUploadBuffer.GetResource());
    // バリア挿入
    cmd->BarrierTransition(
        &m_IntegratedVertexBuffer,
        m_IntegratedVertexBuffer.GetUseState(),
        D3D12_RESOURCE_STATE_COMMON);
    cmd->BarrierTransition(
        &m_IntegratedIndexBuffer,
        m_IntegratedIndexBuffer.GetUseState(),
        D3D12_RESOURCE_STATE_COMMON);
    m_pRenderer->EndComputePass(cmd);
    m_pDevice->m_QueuePool->ReturnQueue(queue);
    vertexUploadBuffer.Destroy();// アップロードバッファ破棄
    indexUploadBuffer.Destroy();// アップロードバッファ破棄
}
