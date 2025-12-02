#include "pch.h"
#include "ColorBuffer.h"

bool ColorBuffer::CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, SUVDescriptorHeap* pDescriptorHeap)
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
		m_SRVHandleIndex = pDescriptorHeap->Allocate();
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

bool ColorBuffer::CreateRTV(ID3D12Device8* device, D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, DescriptorHeap* pDescriptorHeap)
{
	// ヒープがRTVタイプかどうか確認
	if (pDescriptorHeap->GetType() != D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	{
		Log::Write(LogLevel::Assert, "DescriptorHeap is not RTV type");
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
	if (!m_RTVHandleIndex.has_value())
	{
		m_RTVHandleIndex = pDescriptorHeap->Allocate();
	}
	// ディスクリプタヒープがいっぱいの場合は警告を出して終了
	if (!m_RTVHandleIndex.has_value())
	{
		Log::Write(LogLevel::Warn, "DescriptorHeap is full");
		return false;
	}
	// CPUハンドルを取得
	m_RTVCPUHandle = pDescriptorHeap->GetCPUDescriptorHandle(m_RTVHandleIndex.value());
	// Viewの生成
	device->CreateRenderTargetView(
		GetResource(),
		&rtvDesc,
		m_RTVCPUHandle
	);
	return true;
}
