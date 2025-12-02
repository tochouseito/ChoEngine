#include "pch.h"
#include "PixelBuffer.h"

void PixelBuffer::CreatePixelBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES& state)
{
	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// VRAM上に作る
	// パラメータの設定
	m_Width = desc.Width;
	m_Height = desc.Height;
	m_MipLevels = desc.MipLevels;
	m_ArraySize = desc.DepthOrArraySize;
	m_Format = desc.Format;
	m_Dimension = desc.Dimension;
	GpuResource::CreateResource(device, heapProperties, D3D12_HEAP_FLAG_NONE, desc, state, clearValue);
}

void PixelBuffer::RemakePixelBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES& state)
{
	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// VRAM上に作る
	// パラメータの設定
	m_Width = desc.Width;
	m_Height = desc.Height;
	m_MipLevels = desc.MipLevels;
	m_ArraySize = desc.DepthOrArraySize;
	m_Format = desc.Format;
	m_Dimension = desc.Dimension;
	GpuResource::RemakeResource(device, heapProperties, D3D12_HEAP_FLAG_NONE, desc, state, clearValue);
}

bool PixelBuffer::CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, SUVDescriptorHeap* pDescriptorHeap)
{
	// ヒープがSRVタイプかどうか確認
	if (pDescriptorHeap->GetType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		Log::Write(LogLevel::Assert, "DescriptorHeap is not SRV type");
		return false;
	}
	// リソースがあるかどうか確認
	if (!GetResource())
	{
		Log::Write(LogLevel::Assert, "Resource is null");
		return false;
	}
	// 新しいディスクリプタのインデックスとハンドルを取得
	// すでにSRVが作成されている場合はインデックスを取得しない
	if (!m_SRVHandleIndex.has_value())
	{
		m_SRVHandleIndex = pDescriptorHeap->TextureAllocate();
	}
	// ディスクリプタヒープがいっぱいの場合は警告を出して終了
	if (!m_SRVHandleIndex.has_value())
	{
		Log::Write(LogLevel::Warn, "DescriptorHeap is full");
		return false;
	}
	// CPUハンドルを取得
	m_SRVCPUHandle = pDescriptorHeap->GetCPUDescriptorHandle(m_SRVHandleIndex.value());
	// GPUハンドルを取得
	m_SRVGPUHandle = pDescriptorHeap->GetGPUDescriptorHandle(m_SRVHandleIndex.value());
	// Viewの生成
	device->CreateShaderResourceView(
		GetResource(),
		&srvDesc,
		m_SRVCPUHandle
	);
	return true;
}
