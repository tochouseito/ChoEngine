#pragma once

#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"

class DescriptorHeap
{
public:// methods
	// Constructor
	DescriptorHeap(ID3D12Device8* device, const uint32_t& maxDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, const bool& shaderVisible,uint32_t baseIndex=0)
		: m_MaxDescriptor(maxDescriptor), m_Type(heapType), m_NextHandleIndex(baseIndex)
	{
		CreateDescriptor(device, maxDescriptor, heapType, shaderVisible);
	}
	// Destructor
	~DescriptorHeap()
	{

	}
	// GetMaxDescriptor
	virtual uint32_t GetMaxDescriptor() const;
	// GetType
	virtual D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;
	// GetSize
	virtual uint32_t GetSize() const;
	// GetDescriptorHeap
	virtual ID3D12DescriptorHeap* GetDescriptorHeap() const;
	// GetHandle
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const uint32_t& index);
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const uint32_t& index);
	// AllocateHandle
	virtual std::optional<uint32_t> Allocate();
	// RemoveHandle
	virtual bool RemoveHandle(const uint32_t& index);
protected:
	// CreateDescriptor
	void CreateDescriptor(ID3D12Device8* device, const uint32_t& num, D3D12_DESCRIPTOR_HEAP_TYPE type, const bool& shaderVisible);

	// 最大ディスクリプタ数
	uint32_t m_MaxDescriptor;
	// ヒープタイプ
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	// デスクリプタサイズ
	uint32_t m_Size = {};
	// デスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap = nullptr;
	// 次の使用可能なインデックス
	uint32_t m_NextHandleIndex;
	// 返却されたインデックス
	std::vector<uint32_t> m_RemovedHandleIndex = {};
};

// Textureと通常のリソースを分けるSUVHeap
class SUVDescriptorHeap : public DescriptorHeap
{
public:
	// Constructor
	SUVDescriptorHeap(ID3D12Device8* device, const uint32_t& maxDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, const bool& shaderVisible,const uint32_t& maxTextureCount)
		: DescriptorHeap(device, maxDescriptor, heapType, shaderVisible,maxTextureCount), m_MaxTextureCount(maxTextureCount),m_TexNextHandleIndex(0)
	{
	}
	// Destructor
	~SUVDescriptorHeap()
	{
	}
	// AllocateHandle
	std::optional<uint32_t> TextureAllocate();
	// RemoveHandle
	bool TextureRemoveHandle(const uint32_t& index);
private:
	// Texture最大数
	uint32_t m_MaxTextureCount;
	// 次の使用可能なインデックス
	uint32_t m_TexNextHandleIndex;
	// 返却されたインデックス
	std::vector<uint32_t> m_RemovedHandleIndex = {};
};
