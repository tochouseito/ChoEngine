#pragma once

#include "SDK/DirectX/DirectX12/DescriptorHeap/DescriptorHeap.h"

class GpuResource : public std::enable_shared_from_this<GpuResource>
{
public:
    // Constructor
    GpuResource():
		m_UseState(D3D12_RESOURCE_STATE_COMMON)
	{
	}
	// Constructor
    GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
        m_pResource(pResource), m_UseState(CurrentState)
    {

    }
    // Destructor
    virtual ~GpuResource()
    {
        Destroy();
    }
    virtual void Destroy()
    {
        m_pResource.Reset();
		m_UseState = D3D12_RESOURCE_STATE_COMMON;
        ++m_VersionID;
		m_SRVHandleIndex = std::nullopt;
		m_UAVHandleIndex = std::nullopt;
		m_RTVHandleIndex = std::nullopt;
    }
	// 直接リソースを取得する演算子
    ID3D12Resource* operator->() { return m_pResource.Get(); }
    const ID3D12Resource* operator->() const { return m_pResource.Get(); }
	// リソースを取得
    ID3D12Resource* GetResource() { return m_pResource.Get(); }
    const ID3D12Resource* GetResource() const { return m_pResource.Get(); }
	// リソースのポインタを取得
    ID3D12Resource** GetAddressOf() { return m_pResource.GetAddressOf(); }
	// リソースのバージョンIDを取得
    uint32_t GetVersionID() const { return m_VersionID; }
	// ResourceState を取得
	D3D12_RESOURCE_STATES GetResourceState() const { return m_UseState; }
	// ResourceState を設定
	void SetResourceState(D3D12_RESOURCE_STATES state) { m_UseState = state; }
	// ヒープタイプを取得
	D3D12_HEAP_TYPE GetHeapType() const { return m_HeapType; }
	// Viewの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle() const { return m_SRVCPUHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuHandle() const { return m_SRVGPUHandle; }
	std::optional<uint32_t> GetSRVHandleIndex() const { return m_SRVHandleIndex; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCpuHandle() const { return m_UAVCPUHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGpuHandle() const { return m_UAVGPUHandle; }
	std::optional<uint32_t> GetUAVHandleIndex() const { return m_UAVHandleIndex; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpuHandle() const { return m_RTVCPUHandle; }
	std::optional<uint32_t> GetRTVHandleIndex() const { return m_RTVHandleIndex; }

    // 遅延破棄用に自分を shared_ptr 化する
    std::shared_ptr<GpuResource> GetShared()
    {
        return shared_from_this();
    }

	// リソースを作成
    void CreateResource(
        ID3D12Device* device,
        D3D12_HEAP_PROPERTIES& heapProperties,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_DESC& desc,
        D3D12_RESOURCE_STATES InitialState,
        D3D12_CLEAR_VALUE* pClearValue = nullptr);
    // リソースを再生成
	void RemakeResource(
		ID3D12Device* device,
		D3D12_HEAP_PROPERTIES& heapProperties,
		D3D12_HEAP_FLAGS heapFlags,
		D3D12_RESOURCE_DESC& desc,
		D3D12_RESOURCE_STATES InitialState,
		D3D12_CLEAR_VALUE* pClearValue = nullptr);

    // リソースのサイズを変更して再作成
    static void ResizeResource(
        ID3D12Device* device,
        ID3D12Resource** ppResource,
        D3D12_HEAP_PROPERTIES& heapProperties,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_DESC& desc,
        D3D12_RESOURCE_STATES InitialState,
		D3D12_CLEAR_VALUE* pClearValue = nullptr);
protected:
    // リソース
    ComPtr<ID3D12Resource> m_pResource = nullptr;
    // リソースの使用状態
    D3D12_RESOURCE_STATES m_UseState{};
    // 使用ヒープタイプ
    D3D12_HEAP_TYPE m_HeapType{};
    // このリソースのバージョンID。リソースが再利用されるとインクリメントされます。
	uint32_t m_VersionID = 0;
    // View
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVCPUHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_SRVGPUHandle = {};
	std::optional<uint32_t> m_SRVHandleIndex = std::nullopt;

	D3D12_CPU_DESCRIPTOR_HANDLE m_UAVCPUHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_UAVGPUHandle = {};
	std::optional<uint32_t> m_UAVHandleIndex = std::nullopt;

	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVCPUHandle = {};
	std::optional<uint32_t> m_RTVHandleIndex = std::nullopt;
};

// フェンス待ち破棄の構造体
struct PendingDestroyGpuResource
{
    std::shared_ptr<GpuResource> resource;
    std::vector<std::pair<ID3D12Fence*, uint64_t>> fences;// 複数フェンス
};
