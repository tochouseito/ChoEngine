#include "pch.h"
#include "DescriptorHeap.h"

uint32_t DescriptorHeap::GetMaxDescriptor() const
{
	return m_MaxDescriptor;
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeap::GetType() const
{
	return m_Type;
}

uint32_t DescriptorHeap::GetSize() const
{
	return m_Size;
}

ID3D12DescriptorHeap* DescriptorHeap::GetDescriptorHeap() const
{
	return m_DescriptorHeap.Get();
}

std::optional<uint32_t> DescriptorHeap::Allocate()
{
	// 返却されたインデックスがあればそれを使用
	if (!m_RemovedHandleIndex.empty())
	{
		uint32_t result = m_RemovedHandleIndex.back();
		m_RemovedHandleIndex.pop_back();
		return result;
	}
	// 最大数を超えた場合はnullptrを返す
	if (m_MaxDescriptor <= m_NextHandleIndex)
	{
		Log::Write(LogLevel::Warn, "DescriptorHeap is full");
		return std::nullopt;
	}
	// 返却されたインデックスがなければ次のインデックスを使用
	uint32_t result = m_NextHandleIndex;
	m_NextHandleIndex++;
	return result;
}

bool DescriptorHeap::RemoveHandle(const uint32_t& index)
{
	// 返却されたインデックスが最大数を超えた場合はエラー
	if (index >= m_MaxDescriptor)
	{
		Log::Write(LogLevel::Assert, "Invalid index for DescriptorHeap");
		return false;
	}
	// 返却されたインデックスをリサイクルコンテナに追加
	m_RemovedHandleIndex.push_back(index);
	return true;
}

void DescriptorHeap::CreateDescriptor(ID3D12Device8* device, const uint32_t& num, D3D12_DESCRIPTOR_HEAP_TYPE type, const bool& shaderVisible)
{
	m_MaxDescriptor = num;
	m_Type = type;
	m_Size = device->GetDescriptorHandleIncrementSize(type);
	// Create Descriptor Heap
	m_DescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = m_Type;
	desc.NumDescriptors = m_MaxDescriptor;
	desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap));
	Log::Write(LogLevel::Assert, "CreateDescriptorHeap", hr);
}

// GetHandle
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUDescriptorHandle(const uint32_t& index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (m_Size * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUDescriptorHandle(const uint32_t& index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (m_Size* index);
	return handleGPU;
}

std::optional<uint32_t> SUVDescriptorHeap::TextureAllocate()
{
	// 返却されたインデックスがあればそれを使用
	if (!m_RemovedHandleIndex.empty())
	{
		uint32_t result = m_RemovedHandleIndex.back();
		m_RemovedHandleIndex.pop_back();
		return result;
	}
	// 最大数を超えた場合はnullptrを返す
	if (m_MaxTextureCount <= m_TexNextHandleIndex)
	{
		Log::Write(LogLevel::Warn, "DescriptorHeap is full");
		return std::nullopt;
	}
	// 返却されたインデックスがなければ次のインデックスを使用
	uint32_t result = m_TexNextHandleIndex;
	m_TexNextHandleIndex++;
	return result;
}

bool SUVDescriptorHeap::TextureRemoveHandle(const uint32_t& index)
{
	// 返却されたインデックスが最大数を超えた場合はエラー
	if (index >= m_MaxTextureCount)
	{
		Log::Write(LogLevel::Assert, "Invalid index for DescriptorHeap");
		return false;
	}
	// 返却されたインデックスをリサイクルコンテナに追加
	m_RemovedHandleIndex.push_back(index);
	return true;
}
