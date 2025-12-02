#include "pch.h"
#include "ResourceManager.h"
#include "OS/Windows/WinApp/WinApp.h"
#include "SDK/DirectX/DirectX12/SwapChain/SwapChain.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"

ResourceManager::ResourceManager(ID3D12Device8* device)
{
	// ポインタをセット
	m_Device = device;
	// ヒープの作成
	CreateHeap(device);
	// 統合データの作成
	CreateIntegrationBuffers();
	// ダミーマテリアルの作成
	CreateDummyMaterial();
	// ライトバッファの作成
	CreateLightBuffer();
	// 環境バッファの作成
	CreateEnvironmentBuffer();
	// エフェクトパーティクルバッファの作成
	CreateEffectParticleBuffer();
	// EffectRootUseListバッファの作成
	CreateEffectRootUseListBuffer();
	// スクリプトコンテナの作成
	m_ScriptContainer = std::make_unique<ScriptContainer>();
	// 初期化
	Initialize();
}

ResourceManager::~ResourceManager()
{
	Finalize();
}

void ResourceManager::Initialize()
{
	
}

void ResourceManager::Finalize()
{
	// AudioManagerの解放
	if (m_AudioManager)
	{
		m_AudioManager->Finalize();
	}
}

void ResourceManager::Update()
{
}

void ResourceManager::Release()
{
}

void ResourceManager::GenerateManager(GraphicsEngine* graphicsEngine)
{
	m_TextureManager = std::make_unique<TextureManager>(this, graphicsEngine, m_Device);
	m_ModelManager = std::make_unique<ModelManager>(this);
	m_AudioManager = std::make_unique<AudioManager>(this);
	m_UIContainer = std::make_unique<UIContainer>(this);
	m_CommandManager = graphicsEngine->GetCommandManager();
}

uint32_t ResourceManager::CreateColorBuffer(D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state)
{
	// カラーバッファの生成
	uint32_t index = static_cast<uint32_t>(m_ColorBuffers.emplace_back(std::make_shared<ColorBuffer>()));
	std::weak_ptr<ColorBuffer> buffer = m_ColorBuffers[index].load();
	if (auto ptr = buffer.lock())
	{
		ptr->CreatePixelBufferResource(m_Device, desc, clearValue, state);
		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		// Viewの生成
		ptr->CreateSRV(m_Device, srvDesc, m_SUVDescriptorHeap.get());
		// RTVの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = desc.Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		// Viewの生成
		ptr->CreateRTV(m_Device, rtvDesc, m_RTVDescriptorHeap.get());
	}
	return index;
}

uint32_t ResourceManager::CreateDepthBuffer(D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES state)
{
	// 深度バッファの生成
	uint32_t index = static_cast<uint32_t>(m_DepthBuffers.emplace_back(std::make_shared<DepthBuffer>()));
	std::weak_ptr<DepthBuffer> buffer = m_DepthBuffers[index].load();
	if (auto ptr = buffer.lock())
	{
		ptr->CreateDepthBufferResource(m_Device, desc, state);
		// DSVの設定
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = desc.Format;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		// Viewの生成
		ptr->CreateDSV(m_Device, dsvDesc, m_DSVDescriptorHeap.get());
	}
	return index;
}

uint32_t ResourceManager::CreateTextureBuffer(D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state, const bool& isTextureCube)
{
	// テクスチャバッファの生成
	uint32_t index = static_cast<uint32_t>(m_TextureBuffers.emplace_back(std::make_shared<PixelBuffer>()));
	std::weak_ptr<PixelBuffer> buffer = m_TextureBuffers[index].load();
	if (auto ptr = buffer.lock())
	{
		ptr->CreatePixelBufferResource(m_Device, desc, clearValue, state);
		// SRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = isTextureCube ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURE2D;
		if (isTextureCube)
		{
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = UINT_MAX;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			srvDesc.Texture2D.MipLevels = static_cast<UINT>(desc.MipLevels);
		}
		// Viewの生成
		ptr->CreateSRV(m_Device, srvDesc, m_SUVDescriptorHeap.get());
	}
	return index;
}

bool ResourceManager::RemakeColorBuffer(std::optional<uint32_t>& index, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state)
{
	ColorBuffer* buffer = GetBuffer<ColorBuffer>(index);
	// ディスクリプタヒープにインデックスを返却
	if (buffer->GetSRVHandleIndex().has_value())
	{
		m_SUVDescriptorHeap->RemoveHandle(buffer->GetSRVHandleIndex().value());
	}
	if(buffer->GetUAVHandleIndex().has_value())
	{
		m_SUVDescriptorHeap->RemoveHandle(buffer->GetUAVHandleIndex().value());
	}
	if (buffer->GetRTVHandleIndex().has_value())
	{
		m_RTVDescriptorHeap->RemoveHandle(buffer->GetRTVHandleIndex().value());
	}
	buffer->RemakePixelBufferResource(m_Device, desc, clearValue, state);
	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	// Viewの生成
	buffer->CreateSRV(m_Device, srvDesc, m_SUVDescriptorHeap.get());
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = desc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	// Viewの生成
	buffer->CreateRTV(m_Device, rtvDesc, m_RTVDescriptorHeap.get());
	return true;
}

void ResourceManager::CreateIntegrationBuffers()
{
	// Transform統合バッファ
	std::optional<uint32_t> transformIndex = CreateStructuredBuffer<BUFFER_DATA_TF>(kIntegrationTFBufferSize);
	m_IntegrationData[IntegrationDataType::Transform] = std::make_unique<IntegrationData<BUFFER_DATA_TF>>(transformIndex, kIntegrationTFBufferSize);
	// Line統合バッファ
	std::optional<uint32_t> lineIndex = CreateVertexBuffer<BUFFER_DATA_LINE>(kIntegrationLineBufferSize);
	m_IntegrationData[IntegrationDataType::Line] = std::make_unique<IntegrationData<BUFFER_DATA_LINE>>(lineIndex, kIntegrationLineBufferSize);
	// Material統合バッファ
	std::optional<uint32_t> materialIndex = CreateStructuredBuffer<BUFFER_DATA_MATERIAL>(kIntegrationMaterialBufferSize);
	m_IntegrationData[IntegrationDataType::Material] = std::make_unique<IntegrationData<BUFFER_DATA_MATERIAL>>(materialIndex, kIntegrationMaterialBufferSize);
	// UISprite統合バッファ
	std::optional<uint32_t> uiSpriteIndex = CreateStructuredBuffer<BUFFER_DATA_UISPRITE>(kIntegrationUISpriteBufferSize);
	m_IntegrationData[IntegrationDataType::UISprite] = std::make_unique<IntegrationData<BUFFER_DATA_UISPRITE>>(uiSpriteIndex, kIntegrationUISpriteBufferSize);
	// EffectRoot統合バッファ
	std::optional<uint32_t> effectRootIndex = CreateStructuredBuffer<EffectRoot>(kIntegrationEffectRootBufferSize);
	m_IntegrationData[IntegrationDataType::EffectRootInt] = std::make_unique<IntegrationData<EffectRoot>>(effectRootIndex, kIntegrationEffectRootBufferSize);
	// EffectNode統合バッファ
	std::optional<uint32_t> effectNodeIndex = CreateStructuredBuffer<EffectNode>(kIntegrationEffectNodeBufferSize);
	m_IntegrationData[IntegrationDataType::EffectNodeInt] = std::make_unique<IntegrationData<EffectNode>>(effectNodeIndex, kIntegrationEffectNodeBufferSize);
	// EffectSprite統合バッファ
	std::optional<uint32_t> effectSpriteIndex = CreateStructuredBuffer<EffectSprite>(kIntegrationEffectSpriteBufferSize);
	m_IntegrationData[IntegrationDataType::EffectSpriteInt] = std::make_unique<IntegrationData<EffectSprite>>(effectSpriteIndex, kIntegrationEffectSpriteBufferSize);
	// EffectRing統合バッファ
	std::optional<uint32_t> effectRingIndex = CreateStructuredBuffer<EffectRing>(kIntegrationEffectRingBufferSize);
	m_IntegrationData[IntegrationDataType::EffectRingInt] = std::make_unique<IntegrationData<EffectRing>>(effectRingIndex, kIntegrationEffectRingBufferSize);
}

void ResourceManager::CreateLightBuffer()
{
	// ライトバッファの作成
	uint32_t index = CreateConstantBuffer<BUFFER_DATA_LIGHT>();
	m_LightBuffer = dynamic_cast<ConstantBuffer<BUFFER_DATA_LIGHT>*>(GetBuffer<IConstantBuffer>(index));
	// ライトバッファの初期化
	BUFFER_DATA_LIGHT data = {};
	for (uint32_t i = 0;i < kMaxLight;i++)
	{
		data.lightData[i].active = false;
		data.lightData[i].type = static_cast<uint32_t>(LightType::Directional);
	}
}

void ResourceManager::CreateEnvironmentBuffer()
{
	// 環境バッファの作成
	uint32_t index = CreateConstantBuffer<BUFFER_DATA_ENVIRONMENT>();
	m_EnvironmentBuffer = dynamic_cast<ConstantBuffer<BUFFER_DATA_ENVIRONMENT>*>(GetBuffer<IConstantBuffer>(index));
}

// EffectParticleBuffer
void ResourceManager::CreateEffectParticleBuffer()
{
	// エフェクトパーティクルバッファの作成
	uint32_t index = CreateRWStructuredBuffer<EffectParticle>(128 * 1024);
	m_EffectParticleBuffer = dynamic_cast<RWStructuredBuffer<EffectParticle>*>(GetBuffer<IRWStructuredBuffer>(index));
	// FreeList
	uint32_t freeListIndex = CreateRWStructuredBuffer<uint32_t>(128 * 1024,true);
	m_EffectParticleFreeList = dynamic_cast<RWStructuredBuffer<uint32_t>*>(GetBuffer<IRWStructuredBuffer>(freeListIndex));
}

void ResourceManager::CreateEffectRootUseListBuffer()
{
	// EffectRootUseListバッファの作成
	uint32_t index = CreateStructuredBuffer<uint32_t>(128);
	m_EffectRootUseListBuffer = dynamic_cast<StructuredBuffer<uint32_t>*>(GetBuffer<IStructuredBuffer>(index));
}

void ResourceManager::CreateHeap(ID3D12Device8* device)
{
	m_SUVDescriptorHeap = std::make_unique<SUVDescriptorHeap>(device, kMaxSUVDescriptorHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true,kMaxTextureCount);
	m_RTVDescriptorHeap = std::make_unique<DescriptorHeap>(device, kMaxRTVDescriptorHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
	m_DSVDescriptorHeap = std::make_unique<DescriptorHeap>(device, kMaxDSVDescriptorHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
}

void ResourceManager::CreateDummyMaterial()
{
	uint32_t mapID = m_IntegrationData[IntegrationDataType::Material]->GetMapID();
	if (mapID != 0) { Log::Write(LogLevel::Assert, "DummyMaterial Create Fail"); }
	StructuredBuffer<BUFFER_DATA_MATERIAL>* pIntegrationBuffer = dynamic_cast<StructuredBuffer<BUFFER_DATA_MATERIAL>*>(GetIntegrationBuffer(IntegrationDataType::Material));
	// ダミーマテリアルの初期化
	BUFFER_DATA_MATERIAL data = {};
	Color color;
	data.color = color.From255(200, 200, 200);
	/*data.enableLighting = true;
	data.enableTexture = false;
	data.matUV = float4x4::Identity();
	data.shininess = 0.0f;
	data.textureId = 0;*/
	pIntegrationBuffer->UpdateData(data, mapID);
}

// 遅延キューに登録
void ResourceManager::RegisterDeferredDestroy(const std::shared_ptr<GpuResource>& resource)
{
	// 遅延キュー用のFenceを取得
	std::vector<std::pair<ID3D12Fence*, uint64_t>> fences;
	for (auto& queue : m_CommandManager->GetQueueContexts())
	{
		std::pair<ID3D12Fence*, uint64_t> fence = std::make_pair(queue->GetFence(), queue->GetFenceValue());
		fences.push_back(fence);
	}
	m_DeferredDestroyQueue.Register(resource, fences);
}

