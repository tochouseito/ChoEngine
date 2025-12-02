#pragma once

/*--------------------------------------------
リソースマネージャクラス
--------------------------------------------*/

#include "SDK/DirectX/DirectX12/DescriptorHeap/DescriptorHeap.h"
#include "SDK/DirectX/DirectX12/GpuBuffer/GpuBuffer.h"
#include "SDK/DirectX/DirectX12/VertexBuffer/VertexBuffer.h"
#include "SDK/DirectX/DirectX12/IndexBuffer/IndexBuffer.h"
#include "SDK/DirectX/DirectX12/ColorBuffer/ColorBuffer.h"
#include "SDK/DirectX/DirectX12/DepthBuffer/DepthBuffer.h"
#include "Resources/IntegrationData/IntegrationData.h"
#include "Resources/ModelManager/ModelManager.h"
#include "Resources/TextureManager/TextureManager.h"
#include "Resources/ScriptContainer/ScriptContainer.h"
#include "Resources/AudioManager/AudioManager.h"
#include "Core/Utility/CompBufferData.h"
#include "Core/Utility/atomic_shared_ptr.h"
#include "Resources/UIContainer/UIContainer.h"

enum IntegrationDataType
{
	Transform=0,
	Line,
	Material,
	UISprite,
	EffectRootInt,
	EffectNodeInt,
	EffectSpriteInt,
	EffectRingInt,
	kCount,
};

class GraphicsEngine;
class CommandManager;
class SwapChain;

// 遅延破棄キュー
class DeferredDestroyQueue
{
public:
	void Register(const std::shared_ptr<GpuResource>& resource, const std::vector<std::pair<ID3D12Fence*, uint64_t>>& fences)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.push_back({ resource, fences });
	}

	void Process()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto it = m_Queue.begin();
		while (it != m_Queue.end())
		{
			if (IsAllFencesCompleted(*it))
			{
				it = m_Queue.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
private:
	bool IsAllFencesCompleted(const PendingDestroyGpuResource& pending)
	{
		for (const auto& [fence, value] : pending.fences)
		{
			if (fence->GetCompletedValue() < value)
			{
				return false;
			}
		}
		return true;
	}
	std::mutex m_Mutex;
	std::vector<PendingDestroyGpuResource> m_Queue;
};

class ResourceManager
{
	friend class GraphicsEngine;
	friend class ModelManager;
public:
	// コンストラクタ
	ResourceManager(ID3D12Device8* device);
	// デストラクタ
	~ResourceManager();
	// 初期化
	void Initialize();
	// 終了処理
	void Finalize();
	// 更新
	void Update();
	// 解放
	void Release();
	// モデル、テクスチャマネージャー生成
	void GenerateManager(GraphicsEngine* graphicsEngine);
	// CreateBuffer
	template<typename T>
	uint32_t CreateConstantBuffer()
	{
		// 定数バッファの生成
		uint32_t index = static_cast<uint32_t>(m_ConstantBuffers.emplace_back(std::make_shared<ConstantBuffer<T>>()));
		std::weak_ptr<IConstantBuffer> buffer = m_ConstantBuffers[index].load();
		if (auto ptr = buffer.lock())
		{
			ptr->CreateConstantBufferResource(m_Device);
		}
		return index;
	}
	template<typename T>
	uint32_t CreateStructuredBuffer(const UINT& numElements)
	{
		// 構造化バッファの生成
		uint32_t index = static_cast<uint32_t>(m_StructuredBuffers.emplace_back(std::make_shared<StructuredBuffer<T>>()));
		std::weak_ptr<IStructuredBuffer> buffer = m_StructuredBuffers[index].load();
		if (auto ptr = buffer.lock())
		{
			ptr->CreateStructuredBufferResource(m_Device, numElements);
			// SRVの生成
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.NumElements = ptr->GetNumElements();
			srvDesc.Buffer.StructureByteStride = ptr->GetStructureByteStride();
			ptr->CreateSRV(m_Device, srvDesc, m_SUVDescriptorHeap.get());
		}
		return index;
	}
	template<typename T>
	uint32_t CreateRWStructuredBuffer(const UINT& numElements, bool useCounter = false)
	{
		// 構造化バッファの生成
		uint32_t index = static_cast<uint32_t>(m_UAVBuffers.emplace_back(std::make_shared<RWStructuredBuffer<T>>()));
		std::weak_ptr<IRWStructuredBuffer> buffer = m_UAVBuffers[index].load();
		if (auto ptr = buffer.lock())
		{
			ptr->CreateRWStructuredBufferResource(m_Device, numElements);
			// UAVの生成
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			uavDesc.Buffer.NumElements = ptr->GetNumElements();
			uavDesc.Buffer.StructureByteStride = ptr->GetStructureByteStride();
			ptr->CreateUAV(m_Device, uavDesc, m_SUVDescriptorHeap.get(), useCounter);
		}
		return index;
	}
	template<typename T>
	uint32_t CreateVertexBuffer(const UINT& numElements,bool isSkinning = false)
	{
		// 頂点バッファの生成
		uint32_t index = static_cast<uint32_t>(m_VertexBuffers.emplace_back(std::make_shared<VertexBuffer<T>>()));
		std::weak_ptr<IVertexBuffer> buffer = m_VertexBuffers[index].load();
		if(auto ptr = buffer.lock())
		{
			ptr->CreateVertexBufferResource(m_Device, numElements, isSkinning);
			// CreateVBV
			ptr->CreateVBV();
			if (isSkinning)
			{
				// スキニング用SRVを作成
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = 0;
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				srvDesc.Buffer.NumElements = ptr->GetNumElements();
				srvDesc.Buffer.StructureByteStride = ptr->GetStructureByteStride();
				ptr->CreateSRV(m_Device, srvDesc, m_SUVDescriptorHeap.get());
				// スキニング用UAVを作成
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				uavDesc.Buffer.NumElements = ptr->GetNumElements();
				uavDesc.Buffer.StructureByteStride = ptr->GetStructureByteStride();
				ptr->CreateUAV(m_Device, uavDesc, m_SUVDescriptorHeap.get());
			}
		}
		return index;
	}
	template<typename T>
	uint32_t CreateIndexBuffer(const UINT& numElements)
	{
		// インデックスバッファの生成
		uint32_t index = static_cast<uint32_t>(m_IndexBuffers.emplace_back(std::make_shared<IndexBuffer<T>>()));
		std::weak_ptr<IIndexBuffer> buffer = m_IndexBuffers[index].load();
		if(auto ptr = buffer.lock())
		{
			ptr->CreateIndexBufferResource(m_Device, numElements);
			// CreateIBV
			ptr->CreateIBV();
		}
		return index;
	}
	uint32_t CreateColorBuffer(D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state);
	uint32_t CreateDepthBuffer(D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES state);
	uint32_t CreateTextureBuffer(D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state,const bool& isTextureCube = false);

	// ResizeBuffer
	template<typename T>
	bool ResizeStructuredBuffer(const uint32_t& idx, const UINT& numElements)
	{
		// 新しい構造化バッファの生成
		std::shared_ptr<StructuredBuffer<T>> newBuffer = std::make_shared<StructuredBuffer<T>>();
		newBuffer->CreateStructuredBufferResource(m_Device, numElements);
		// SRVの生成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.NumElements = newBuffer->GetNumElements();
		srvDesc.Buffer.StructureByteStride = newBuffer->GetStructureByteStride();
		newBuffer->CreateSRV(m_Device, srvDesc, m_SUVDescriptorHeap.get());
		// 既存のバッファを置き換え、古いポインタを取得
		std::shared_ptr<T> oldBuffer = m_StructuredBuffers[idx].exchange(newBuffer);
		// 遅延キューに登録
		RegisterDeferredDestroy(oldBuffer);
	}

	// RemakeBuffer
	bool RemakeColorBuffer(std::optional<uint32_t>& index, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state);

	// ReleaseBuffer

	// 統合バッファ
	void CreateIntegrationBuffers();
	IStructuredBuffer* GetIntegrationBuffer(const IntegrationDataType & type) const
	{
		switch (type)
		{
		case IntegrationDataType::Transform:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::Transform]->GetBufferIndex());
			break;
		case IntegrationDataType::Material:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::Material]->GetBufferIndex());
			break;
		case IntegrationDataType::UISprite:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::UISprite]->GetBufferIndex());
			break;
		case IntegrationDataType::EffectRootInt:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::EffectRootInt]->GetBufferIndex());
			break;
		case IntegrationDataType::EffectNodeInt:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::EffectNodeInt]->GetBufferIndex());
			break;
		case IntegrationDataType::EffectSpriteInt:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::EffectSpriteInt]->GetBufferIndex());
			break;
		case IntegrationDataType::EffectRingInt:
			return GetBuffer<IStructuredBuffer>(m_IntegrationData[IntegrationDataType::EffectRingInt]->GetBufferIndex());
			break;
		default:
			break;
		}
		return nullptr;
	}
	VertexBuffer<BUFFER_DATA_LINE>* GetLineIntegrationBuffer()const
	{
		return dynamic_cast<VertexBuffer<BUFFER_DATA_LINE>*>(GetBuffer<IVertexBuffer>(m_IntegrationData[IntegrationDataType::Line]->GetBufferIndex()));
	}

	// GetBuffer
	template<typename T>
	T* GetBuffer(const std::optional<uint32_t>& index) const
	{
		if (!index.has_value())
		{
			return nullptr;
		}
		if constexpr (std::is_same_v<T, ColorBuffer>)
		{
			return m_ColorBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, DepthBuffer>)
		{
			return m_DepthBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, IVertexBuffer>)
		{
			return m_VertexBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, IIndexBuffer>)
		{
			return m_IndexBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, IConstantBuffer>)
		{
			return m_ConstantBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, IStructuredBuffer>)
		{
			return m_StructuredBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, PixelBuffer>)
		{
			return m_TextureBuffers[index.value()].load().get();
		} else if constexpr (std::is_same_v<T, IRWStructuredBuffer>)
		{
			return m_UAVBuffers[index.value()].load().get();
		}else
			assert(false && "Invalid buffer type");
	}

	//Getters
	SUVDescriptorHeap* GetSUVDHeap() const { return m_SUVDescriptorHeap.get(); }
	DescriptorHeap* GetRTVDHeap() const { return m_RTVDescriptorHeap.get(); }
	DescriptorHeap* GetDSVDHeap() const { return m_DSVDescriptorHeap.get(); }
	TextureManager* GetTextureManager() const { return m_TextureManager.get(); }
	ModelManager* GetModelManager() const { return m_ModelManager.get(); }
	ScriptContainer* GetScriptContainer() const { return m_ScriptContainer.get(); }
	AudioManager* GetAudioManager() const { return m_AudioManager.get(); }
	UIContainer* GetUIContainer() const { return m_UIContainer.get(); }
	IIntegrationData* GetIntegrationData(const IntegrationDataType& type) const
	{
		switch (type)
		{
		case IntegrationDataType::Transform:
			return m_IntegrationData[IntegrationDataType::Transform].get();
			break;
		case IntegrationDataType::Line:
			return m_IntegrationData[IntegrationDataType::Line].get();
			break;
		case IntegrationDataType::Material:
			return m_IntegrationData[IntegrationDataType::Material].get();
			break;
		case IntegrationDataType::UISprite:
			return m_IntegrationData[IntegrationDataType::UISprite].get();
			break;
		case IntegrationDataType::EffectRootInt:
			return m_IntegrationData[IntegrationDataType::EffectRootInt].get();
			break;
		case IntegrationDataType::EffectNodeInt:
			return m_IntegrationData[IntegrationDataType::EffectNodeInt].get();
			break;
		case IntegrationDataType::EffectSpriteInt:
			return m_IntegrationData[IntegrationDataType::EffectSpriteInt].get();
			break;
		default:
			break;
		}
		return nullptr;
	}
	// DebugCameraBuffer
	void SetDebugCameraBuffer(ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>* buffer)
	{
		m_DebugCameraBuffer = buffer;
	}
	ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>* GetDebugCameraBuffer() const
	{
		return m_DebugCameraBuffer;
	}

	// LightBuffer
	void CreateLightBuffer();
	ConstantBuffer<BUFFER_DATA_LIGHT>* GetLightBuffer() const
	{
		return m_LightBuffer;
	}
	// LightIndex
	uint32_t GetLightIndex()
	{
		if (m_LightIndexRecycle.empty())
		{
			return m_NextLightIndex++;
		}
		uint32_t index = m_LightIndexRecycle.back();
		m_LightIndexRecycle.pop_back();
		return index;
	}
	void RecycleLightIndex(const uint32_t& index)
	{
		m_LightIndexRecycle.push_back(index);
	}
	// EnvironmentBuffer
	void CreateEnvironmentBuffer();
	ConstantBuffer<BUFFER_DATA_ENVIRONMENT>* GetEnvironmentBuffer() const
	{
		return m_EnvironmentBuffer;
	}
	// EffectParticleBuffer
	void CreateEffectParticleBuffer();
	RWStructuredBuffer<EffectParticle>* GetEffectParticleBuffer() const
	{
		return m_EffectParticleBuffer;
	}
	// EffectParticleFreeListBuffer
	RWStructuredBuffer<uint32_t>* GetEffectParticleFreeListBuffer() const
	{
		return m_EffectParticleFreeList;
	}
	// EffectRootUseListBuffer
	void CreateEffectRootUseListBuffer();
	StructuredBuffer<uint32_t>* GetEffectRootUseListBuffer() const
	{
		return m_EffectRootUseListBuffer;
	}
	// EffectRootUseListIndex
	uint32_t GetEffectRootUseListIndex()
	{
		if (m_EffectRootUseListRecycle.empty())
		{
			return m_NextEffectRootUseListIndex++;
		}
		uint32_t index = m_EffectRootUseListRecycle.back();
		m_EffectRootUseListRecycle.pop_back();
		return index;
	}
	void RecycleEffectRootUseListIndex(const uint32_t& index)
	{
		m_EffectRootUseListRecycle.push_back(index);
	}
	size_t GetEffectRootUseListCount() const
	{
		return m_EffectRootUseList.size();
	}
	void AddEffectRootUseList(const uint32_t& index)
	{
		m_EffectRootUseList.push_back(index);
		// 更新
		uint32_t a = 0;
		for(const auto& i : m_EffectRootUseList)
		{
			m_EffectRootUseListBuffer->UpdateData(i, a);
		}
	}
	void RemoveEffectRootUseList(const uint32_t& index)
	{
		m_EffectRootUseList.remove(index);
		// 更新
		uint32_t a = 0;
		for (const auto& i : m_EffectRootUseList)
		{
			m_EffectRootUseListBuffer->UpdateData(i, a);
		}
	}
	std::wstring GetSkyboxTextureName() const
	{
		return m_SkyboxTextureName;
	}
	void SetSkyboxTextureName(const std::wstring& name)
	{
		m_SkyboxTextureName = name;
	}
private:
	// Heap生成
	void CreateHeap(ID3D12Device8* device);
	static const uint32_t kMaxSUVDescriptorHeapSize = 1024;
	static const uint32_t kMaxRTVDescriptorHeapSize = 20;
	static const uint32_t kMaxDSVDescriptorHeapSize = 2;
	// ダミーマテリアル作成
	void CreateDummyMaterial();
	// 遅延キューに登録
	void RegisterDeferredDestroy(const std::shared_ptr<GpuResource>& resource);
	
	ID3D12Device* GetDevice() const { return m_Device; }

	// Device
	ID3D12Device8* m_Device = nullptr;
	// CommnadManager
	CommandManager* m_CommandManager = nullptr;
	// SUVディスクリプタヒープ
	std::unique_ptr<SUVDescriptorHeap> m_SUVDescriptorHeap = nullptr;
	// RTVディスクリプタヒープ
	std::unique_ptr<DescriptorHeap> m_RTVDescriptorHeap = nullptr;
	// DSVディスクリプタヒープ
	std::unique_ptr<DescriptorHeap> m_DSVDescriptorHeap = nullptr;
	// テクスチャマネージャ
	std::unique_ptr<TextureManager> m_TextureManager = nullptr;
	// モデルマネージャ
	std::unique_ptr<ModelManager> m_ModelManager = nullptr;
	// スクリプトコンテナ
	std::unique_ptr<ScriptContainer> m_ScriptContainer = nullptr;
	// オーディオマネージャ
	std::unique_ptr<AudioManager> m_AudioManager = nullptr;
	// UIコンテナ
	std::unique_ptr<UIContainer> m_UIContainer = nullptr;
	// 遅延破棄キュー
	DeferredDestroyQueue m_DeferredDestroyQueue;
	// 定数バッファ
	FVector<theatria::atomic_shared_ptr<IConstantBuffer>> m_ConstantBuffers;
	// 構造化バッファ
	FVector<theatria::atomic_shared_ptr<IStructuredBuffer>> m_StructuredBuffers;
	// 頂点バッファ
	FVector<theatria::atomic_shared_ptr<IVertexBuffer>> m_VertexBuffers;
	// インデックスバッファ
	FVector<theatria::atomic_shared_ptr<IIndexBuffer>> m_IndexBuffers;
	// カラーバッファ
	FVector<theatria::atomic_shared_ptr<ColorBuffer>> m_ColorBuffers;
	// 深度バッファ
	FVector<theatria::atomic_shared_ptr<DepthBuffer>> m_DepthBuffers;
	// テクスチャバッファ
	FVector<theatria::atomic_shared_ptr<PixelBuffer>> m_TextureBuffers;
	// UAVバッファ
	FVector<theatria::atomic_shared_ptr<IRWStructuredBuffer>> m_UAVBuffers;
	// 統合バッファ
	std::array<std::unique_ptr<IIntegrationData>, IntegrationDataType::kCount> m_IntegrationData;
	// デバッグカメラバッファ
	ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>* m_DebugCameraBuffer = nullptr;
	// ライトバッファ
	ConstantBuffer<BUFFER_DATA_LIGHT>* m_LightBuffer = nullptr;
	// ライト用
	uint32_t m_NextLightIndex = 0;
	std::vector<uint32_t> m_LightIndexRecycle;
	// 環境情報バッファ
	ConstantBuffer<BUFFER_DATA_ENVIRONMENT>* m_EnvironmentBuffer = nullptr;
	// EffectParticleバッファ
	RWStructuredBuffer<EffectParticle>* m_EffectParticleBuffer = nullptr;
	// EffectParticleFreeList
	RWStructuredBuffer<uint32_t>* m_EffectParticleFreeList = nullptr;
	// EffectRootUseListBuffer
	StructuredBuffer<uint32_t>* m_EffectRootUseListBuffer = nullptr;
	// RootUseList
	std::list<uint32_t> m_EffectRootUseList;
	// RootUseList用
	uint32_t m_NextEffectRootUseListIndex = 0;
	// EffectRootUseListのリサイクル用
	std::vector<uint32_t> m_EffectRootUseListRecycle;
	// SkyboxTexture
	std::wstring m_SkyboxTextureName = L"";

	// static member
	static const uint32_t kIntegrationTFBufferSize = 1024;// Transformの統合バッファのサイズ
	static const uint32_t kIntegrationLineBufferSize = 1024;// Lineの統合バッファのサイズ
	static const uint32_t kIntegrationMaterialBufferSize = 1024;// Materialの統合バッファのサイズ
	static const uint32_t kIntegrationUISpriteBufferSize = 128;// UISpriteの統合バッファのサイズ
	static const uint32_t kIntegrationEffectRootBufferSize = 128;// EffectRootの統合バッファのサイズ
	static const uint32_t kIntegrationEffectNodeBufferSize = 1024;// EffectNodeの統合バッファのサイズ
	static const uint32_t kIntegrationEffectSpriteBufferSize = 1024;// EffectSpriteの統合バッファのサイズ
	static const uint32_t kIntegrationEffectRingBufferSize = 1024;// EffectRingの統合バッファのサイズ
	// Texture最大数
	static const uint32_t kMaxTextureCount = 256;
};

