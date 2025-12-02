#pragma once
#include "SDK/DirectX/DirectX12/PixelBuffer/PixelBuffer.h"

class DepthBuffer : public PixelBuffer
{
public:
	// Constructor
	DepthBuffer()
		: PixelBuffer()
	{
	}
	// Constructor
	DepthBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState)
		: PixelBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	~DepthBuffer()
	{
		m_DSVCpuHandle = {};
		m_DSVHandleIndex = std::nullopt;
	}
	// デプスバッファの生成
	void CreateDepthBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES& state);
	// デプスバッファの再生成
	void RemakeDepthBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES state);
	// デプスバッファのSRVを生成
	bool CreateDSV(ID3D12Device8* device, D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, DescriptorHeap* pDescriptorHeap);
	// ディスクリプタハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() const { return m_DSVCpuHandle; }
	// ディスクリプタハンドルインデックスを取得
	std::optional<uint32_t> GetDSVHandleIndex() const { return m_DSVHandleIndex; }
private:
	// ディスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVCpuHandle = {};
	// ディスクリプタハンドルインデックス
	std::optional<uint32_t> m_DSVHandleIndex = std::nullopt;
};

