#include "pch.h"
#include "DepthBuffer.h"

void DepthBuffer::CreateDepthBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES& state)
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
	// Resourceの設定
	// DepthStencilとして使う通知
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	// 深度値のクリア設定
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.DepthStencil.Depth = 1.0f;// 1.0f（最大値）でクリア
	clearValue.Format = desc.Format;// フォーマット。Resourceと合わせる
	PixelBuffer::CreatePixelBufferResource(device, desc, &clearValue, state);
}

void DepthBuffer::RemakeDepthBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES state)
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
	// Resourceの設定
	// DepthStencilとして使う通知
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	// 深度値のクリア設定
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.DepthStencil.Depth = 1.0f;// 1.0f（最大値）でクリア
	clearValue.Format = desc.Format;// フォーマット。Resourceと合わせる
	PixelBuffer::RemakePixelBufferResource(device, desc, &clearValue, state);
}

bool DepthBuffer::CreateDSV(ID3D12Device8* device, D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, DescriptorHeap* pDescriptorHeap)
{
	// ヒープがDSVタイプかどうか確認
	if (pDescriptorHeap->GetType() != D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
	{
		Log::Write(LogLevel::Assert, "DescriptorHeap is not DSV type");
		return false;
	}
	// リソースがあるかどうか確認
	if (!GetResource())
	{
		Log::Write(LogLevel::Assert, "Resource is null");
		return false;
	}
	// 新しいディスクリプタのインデックスとハンドルを取得
	// すでにDSVが作成されている場合はインデックスを取得しない
	if (!m_DSVHandleIndex.has_value())
	{
		m_DSVHandleIndex = pDescriptorHeap->Allocate();
	}
	// ディスクリプタヒープがいっぱいの場合は警告を出して終了
	if (!m_DSVHandleIndex.has_value())
	{
		Log::Write(LogLevel::Warn, "DescriptorHeap is full");
		return false;
	}
	// CPUハンドルを取得
	m_DSVCpuHandle = pDescriptorHeap->GetCPUDescriptorHandle(m_DSVHandleIndex.value());
	// Viewの生成
	device->CreateDepthStencilView(
		GetResource(),
		&dsvDesc,
		m_DSVCpuHandle
	);
	return true;
}
