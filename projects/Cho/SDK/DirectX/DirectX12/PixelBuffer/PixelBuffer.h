#pragma once

#include "SDK/DirectX/DirectX12/GpuResource/GpuResource.h"

class PixelBuffer : public GpuResource
{
public:
	// Constructor
	PixelBuffer() : 
		m_Width(0), 
		m_Height(0), 
		m_MipLevels(0),
		m_ArraySize(0),
		m_Format(DXGI_FORMAT_UNKNOWN),
		m_Dimension(D3D12_RESOURCE_DIMENSION_UNKNOWN),
		GpuResource()
	{
	}
	// Constructor
	PixelBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) : 
		m_Width(0),
		m_Height(0),
		m_MipLevels(0),
		m_ArraySize(0),
		m_Format(DXGI_FORMAT_UNKNOWN),
		m_Dimension(D3D12_RESOURCE_DIMENSION_UNKNOWN),
		GpuResource(pResource,CurrentState)
	{
	}
	// Destructor
	~PixelBuffer()
	{
		m_Width = 0;
		m_Height = 0;
		m_MipLevels = 0;
		m_ArraySize = 0;
		m_Format = DXGI_FORMAT_UNKNOWN;
		m_Dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}
	// リソースの生成
	void CreatePixelBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES& state);
	// リソースの再生成
	void RemakePixelBufferResource(ID3D12Device8* device, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES& state);
	// SRVの生成
	virtual bool CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, SUVDescriptorHeap* pDescriptorHeap);
	// ピクセルバッファの幅を取得
	UINT64 GetWidth() const { return m_Width; }
	// ピクセルバッファの高さを取得
	UINT GetHeight() const { return m_Height; }
	// ピクセルバッファのフォーマットを取得
	const DXGI_FORMAT& GetFormat() const { return m_Format; }
	// ピクセルバッファの次元を取得
	D3D12_RESOURCE_DIMENSION GetDimension() const { return m_Dimension; }
protected:
	// ピクセルバッファの幅
	UINT64 m_Width;
	// ピクセルバッファの高さ
	UINT m_Height;
	// ミップマップの数
	UINT16 m_MipLevels;
	// ピクセルバッファの配列数
	UINT16 m_ArraySize;
	// ピクセルバッファのフォーマット
	DXGI_FORMAT m_Format;
	// ピクセルバッファの次元
	D3D12_RESOURCE_DIMENSION m_Dimension;
};

