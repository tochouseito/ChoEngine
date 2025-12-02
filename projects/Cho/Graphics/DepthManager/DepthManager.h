#pragma once
#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"
class ResourceManager;
class DepthManager
{
	friend class GraphicsEngine;
public:
	// Constructor
	DepthManager()
	{
	}
	// Destructor
	~DepthManager() = default;
	void ResizeDepthBuffer(ID3D12Device8* device, ResourceManager* resourceManager, const UINT64& width, const UINT& height);
	// Getter
	uint32_t GetDepthBufferIndex() const { return m_DepthBufferIndex; }
	uint32_t GetDebugDepthBufferIndex() const { return m_DebugDepthBufferIndex; }
	// Setter
	void SetDepthBufferIndex(const uint32_t& index) { m_DepthBufferIndex = index; }
	void SetDebugDepthBufferIndex(const uint32_t& index) { m_DebugDepthBufferIndex = index; }
private:
	uint32_t m_DepthBufferIndex = 0;
	uint32_t m_DebugDepthBufferIndex = 1;
};

