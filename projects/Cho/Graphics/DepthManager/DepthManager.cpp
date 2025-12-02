#include "pch.h"
#include "DepthManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

void DepthManager::ResizeDepthBuffer(ID3D12Device8* device, ResourceManager* resourceManager, const UINT64& width, const UINT& height)
{
	// Resourceの設定
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = resourceDesc.Format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	{// ゲーム用
		DepthBuffer* depthBuffer = resourceManager->GetBuffer<DepthBuffer>(m_DepthBufferIndex);
		// DepthBufferのリサイズ
		depthBuffer->RemakeDepthBufferResource(device, resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		// Viewの生成
		depthBuffer->CreateDSV(device, dsvDesc, resourceManager->GetDSVDHeap());
	}
	{// エディタ用
		DepthBuffer* depthBuffer = resourceManager->GetBuffer<DepthBuffer>(m_DebugDepthBufferIndex);
		// DepthBufferのリサイズ
		depthBuffer->RemakeDepthBufferResource(device, resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		// Viewの生成
		depthBuffer->CreateDSV(device, dsvDesc, resourceManager->GetDSVDHeap());
	}
}
