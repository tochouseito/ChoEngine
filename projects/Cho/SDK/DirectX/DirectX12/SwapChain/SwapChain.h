#pragma once

#include "SDK/DirectX/DirectX12/stdafx/stdafx.h"

class CommandManager;
class GraphicsEngine;
class ResourceManager;

static const uint32_t bufferCount = 2;
struct SwapChainBuffer
{
	ComPtr<ID3D12Resource> pResource;
	std::optional<uint32_t> backBufferIndex = std::nullopt;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVCpuHandle = {};
	std::optional<uint32_t> m_RTVHandleIndex = std::nullopt;
	D3D12_RESOURCE_STATES m_ResourceState = D3D12_RESOURCE_STATE_PRESENT;
};
class SwapChain
{
public:
	// Constructor
	SwapChain(IDXGIFactory7* dxgiFactory, GraphicsEngine* graphicsEngine, ID3D12CommandQueue* queue, CommandManager* commandManagerPtr, const HWND& hwnd, const UINT64& width, const UINT& height);
	// Destructor
	~SwapChain()
	{

	}
	// SwapChainの生成
	void CreateSwapChain(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* queue, const HWND& hwnd, const UINT64& width, const UINT& height);
	// Resourceの生成
	void CreateResource(ID3D12Device8* device, ResourceManager* resourceManager);
	// Present
	void Present();
	// Resize
	void Resize(ID3D12Device8* device, const UINT64& width, const UINT& height);
	SwapChainBuffer* GetBuffer(const uint32_t& index) { return m_BufferData[index].get(); }
	// GetBackBufferIndex
	UINT GetCurrentBackBufferIndex() const { return m_SwapChain->GetCurrentBackBufferIndex(); }
private:
	ComPtr<IDXGISwapChain4> m_SwapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 m_Desc = {};
	int32_t m_RefreshRate = {};
	// BufferIndex
	std::array<std::unique_ptr<SwapChainBuffer>, bufferCount> m_BufferData = {};

	GraphicsEngine* m_GraphicsEngine = nullptr;
	CommandManager* m_CommandManager = nullptr;
};

