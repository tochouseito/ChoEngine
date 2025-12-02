#pragma once
#include "SDK/DirectX/DirectX12/PixelBuffer/PixelBuffer.h"

class ColorBuffer : public PixelBuffer
{
public:
	// Constructor
	ColorBuffer()
		: PixelBuffer()
	{
	}
	// Constructor
	ColorBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState)
		: PixelBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	~ColorBuffer()
	{
	}
	bool CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, SUVDescriptorHeap* pDescriptorHeap) override;
	bool CreateRTV(ID3D12Device8* device, D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, DescriptorHeap* pDescriptorHeap);
};

