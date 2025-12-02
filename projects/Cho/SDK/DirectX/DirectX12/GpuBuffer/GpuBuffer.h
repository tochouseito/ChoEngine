#pragma once

#include "SDK/DirectX/DirectX12/GpuResource/GpuResource.h"
#include <type_traits>

class GpuBuffer : public GpuResource
{
public:
	// Constructor
	GpuBuffer() 
	{
	}
	// Constructor
	GpuBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		GpuResource(pResource, CurrentState)
	{
	}
	// Destructor
	virtual ~GpuBuffer()
	{
		
	}
	// CreateBuffer
	void CreateBuffer(
		ID3D12Device* device,
		D3D12_HEAP_PROPERTIES& heapProperties,
		D3D12_HEAP_FLAGS heapFlags,
		D3D12_RESOURCE_STATES InitialState,
		D3D12_RESOURCE_FLAGS resourceFlags,
		const UINT& numElements,
		const UINT& structureByteStride);
	// Getters
	UINT64 GetBufferSize() const { return m_BufferSize; }
	UINT GetNumElements() const { return m_NumElements; }
	UINT GetStructureByteStride() const { return m_StructureByteStride; }
protected:
    UINT64 m_BufferSize = {};
    UINT m_NumElements = {};
    UINT m_StructureByteStride = {};
};

// 定数バッファのインターフェース
class IConstantBuffer : public GpuBuffer 
{
public:
	// Constructor
	IConstantBuffer() : GpuBuffer()
	{
	}
	// Constructor
	IConstantBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		GpuBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	virtual ~IConstantBuffer() = default;
	// リソース作成
	virtual void CreateConstantBufferResource(ID3D12Device* device) = 0;
	// マッピング
	virtual void Unmap() = 0;
};

template<typename T>
class ConstantBuffer : public IConstantBuffer
{
public:
	// Constructor
	ConstantBuffer() : IConstantBuffer()
	{
	}
	// Constructor
	ConstantBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		IConstantBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	~ConstantBuffer()
	{
		m_MappedData = std::span<T>{};
	}
	// リソースを作成
	void CreateConstantBufferResource(ID3D12Device* device) override
	{
		// 構造体のサイズを確認
		//if constexpr (std::is_class_v<T>)// クラス、構造体用
		//{
		//	size_t size = sizeof(T);
		//	bool isValid = size % 16 != 0;
		//	if (isValid)
		//	{
		//		Log::Write(LogLevel::Assert, "Structure size must be multiple of 16 bytes");
		//	}
		//}
		// リソースのサイズ
		UINT structureByteStride = static_cast<UINT>(sizeof(T));
		// リソース用のヒープの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;// UploadHeapを使う
		GpuBuffer::CreateBuffer(
			device, heapProperties, D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_FLAG_NONE,
			1, structureByteStride);
		// マッピング
		T* mappedData = nullptr;// 一時マップ用
		size_t bufferSize = 1;
		GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		// マップしたデータをspanに変換
		m_MappedData = std::span<T>(mappedData, bufferSize);
	}
	void Unmap() override
	{
		if (GetResource())
		{
			GetResource()->Unmap(0, nullptr);
		}
	}
	void UpdateData(const T& data)
	{
		if (m_MappedData.size() > 0)
		{
			m_MappedData[0] = data;
		} else
		{
			Log::Write(LogLevel::Assert, "Index out of range");
		}
	}
	T& GetData()
	{
		if (m_MappedData.size() > 0)
		{
			return m_MappedData[0];
		} else
		{
			Log::Write(LogLevel::Assert, "Index out of range");
			return m_MappedData[0];
		}
	}
private:
	// マッピングデータ
	std::span<T> m_MappedData;
};

// 構造化バッファのインターフェース
class IStructuredBuffer : public GpuBuffer
{
public:
	// Constructor
	IStructuredBuffer() : GpuBuffer()
	{
	}
	// Constructor
	IStructuredBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		GpuBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	virtual ~IStructuredBuffer() = default;
	// リソース作成
	virtual void CreateStructuredBufferResource(ID3D12Device* device, const UINT& numElements) = 0;
	// SRV作成
	virtual bool CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, DescriptorHeap* pDescriptorHeap) = 0;
};

template<typename T>
class StructuredBuffer : public IStructuredBuffer
{
public:
	// Constructor
	StructuredBuffer() : IStructuredBuffer()
	{
	}
	// Constructor
	StructuredBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		IStructuredBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	~StructuredBuffer()
	{
		m_MappedData = std::span<T>{};
	}
	void CreateStructuredBufferResource(ID3D12Device* device, const UINT& numElements) override
	{
		// リソースのサイズ
		UINT structureByteStride = static_cast<UINT>(sizeof(T));
		// リソース用のヒープの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;// UploadHeapを使う
		GpuBuffer::CreateBuffer(
			device, heapProperties, D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_FLAG_NONE,
			numElements, structureByteStride);
		// マッピング
		T* mappedData = nullptr;// 一時マップ用
		GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
		// マップしたデータをspanに変換
		m_MappedData = std::span<T>(mappedData, static_cast<size_t>(numElements));
	}
	bool CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, DescriptorHeap* pDescriptorHeap) override
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
	void Unmap()
	{
		if (GetResource())
		{
			GetResource()->Unmap(0, nullptr);
		}
	}
	void UpdateData(const T& data,const uint32_t& index)
	{
		if (m_MappedData.size() > index)
		{
			m_MappedData[index] = data;
		} else
		{
			Log::Write(LogLevel::Assert, "Index out of range");
		}
	}
	T& GetData(const uint32_t& index)
	{
		if (m_MappedData.size() > index)
		{
			return m_MappedData[index];
		} else
		{
			Log::Write(LogLevel::Assert, "Index out of range");
			return m_MappedData[0];
		}
	}
	std::span<T> GetMappedData() const
	{
		return m_MappedData;
	}
private:
	// マッピングデータ
	std::span<T> m_MappedData;
};

// UAVのインターフェース
class IRWStructuredBuffer : public GpuBuffer
{
public:
	// Constructor
	IRWStructuredBuffer() : GpuBuffer()
	{
	}
	// Constructor
	IRWStructuredBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		GpuBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	virtual ~IRWStructuredBuffer() = default;
	// リソース作成
	virtual void CreateRWStructuredBufferResource(ID3D12Device8* device, const UINT& numElements) = 0;
	// SRV作成
	virtual bool CreateUAV(ID3D12Device8* device, D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, DescriptorHeap* pDescriptorHeap, bool useCounter = false) = 0;
	// カウンターリソースを取得
	virtual ID3D12Resource* GetCounterResource() { return m_CounterResource.GetResource(); }
	// カウンターリソースの値用
	virtual ID3D12Resource* GetCounterValueResource() { return m_CounterValueResource.GetResource(); }
protected:
	GpuResource m_CounterResource;// カウンターリソース
	GpuResource m_CounterValueResource;// カウンターリソースの値用
};

// UAVのクラス
template<typename T>
class RWStructuredBuffer : public IRWStructuredBuffer
{
public:
	// Constructor
	RWStructuredBuffer() : IRWStructuredBuffer()
	{
	}
	// Constructor
	RWStructuredBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		IRWStructuredBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	~RWStructuredBuffer()
	{
	}
	void CreateRWStructuredBufferResource(ID3D12Device8* device, const UINT& numElements) override
	{
		UINT structureByteStride = static_cast<UINT>(sizeof(T));
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
		GpuBuffer::CreateBuffer(
			device, heapProperties, D3D12_HEAP_FLAG_NONE,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			numElements, structureByteStride);
	}
	bool CreateUAV(ID3D12Device8* device, D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, DescriptorHeap* pDescriptorHeap, bool useCounter = false) override
	{
		// ヒープがUAVタイプかどうか確認
		if (pDescriptorHeap->GetType() != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		{
			Log::Write(LogLevel::Assert, "DescriptorHeap is not UAV type");
			return false;
		}
		// リソースがあるかどうか確認
		if (!GetResource())
		{
			Log::Write(LogLevel::Assert, "Resource is null");
			return false;
		}
		// 新しいディスクリプタのインデックスとハンドルを取得
		if (!m_UAVHandleIndex.has_value())
		{
			m_UAVHandleIndex = pDescriptorHeap->Allocate();
		}
		if (!m_UAVHandleIndex.has_value())
		{
			Log::Write(LogLevel::Warn, "DescriptorHeap is full");
			return false;
		}
		m_UAVCPUHandle = pDescriptorHeap->GetCPUDescriptorHandle(m_UAVHandleIndex.value());
		m_UAVGPUHandle = pDescriptorHeap->GetGPUDescriptorHandle(m_UAVHandleIndex.value());

		if (useCounter)
		{
			// カウンターを使用する場合はカウンターリソースを作成
			uavDesc.Buffer.CounterOffsetInBytes = 0;
			// UAVカウンターリソース
			D3D12_HEAP_PROPERTIES heapProperties{};
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// GPU専用
			// リソースの設定
			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Width = 4;// リソースのサイズ
			// バッファリソースの設定
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			m_CounterResource.CreateResource(
				device,
				heapProperties,
				D3D12_HEAP_FLAG_NONE,
				resourceDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr);
			// カウンター値を取得する用のリソース
			{
				D3D12_HEAP_PROPERTIES heapProps = {};
				heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

				D3D12_RESOURCE_DESC bufferDesc = {};
				bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				bufferDesc.Width = sizeof(UINT);
				bufferDesc.Height = 1;
				bufferDesc.DepthOrArraySize = 1;
				bufferDesc.MipLevels = 1;
				bufferDesc.SampleDesc.Count = 1;
				bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				m_CounterValueResource.CreateResource(
					device,
					heapProps,
					D3D12_HEAP_FLAG_NONE,
					bufferDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr);
			}

			// UAVを作成
			device->CreateUnorderedAccessView(
				GetResource(),
				m_CounterResource.GetResource(),
				&uavDesc,
				m_UAVCPUHandle);
		} else
		{
			device->CreateUnorderedAccessView(
				GetResource(),
				nullptr,
				&uavDesc,
				m_UAVCPUHandle
			);
		}
		return true;
	}
	void Unmap()
	{
		if (GetResource())
		{
			GetResource()->Unmap(0, nullptr);
		}
	}
};
