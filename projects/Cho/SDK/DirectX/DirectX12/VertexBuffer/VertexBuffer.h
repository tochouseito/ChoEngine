#pragma once
#include "SDK/DirectX/DirectX12/GpuBuffer/GpuBuffer.h"

// 頂点バッファのインタフェース
class IVertexBuffer : public GpuBuffer
{
public:
	// Constructor
	IVertexBuffer() : GpuBuffer()
	{
	}
	// Constructor
	IVertexBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		GpuBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	virtual ~IVertexBuffer() = default;
	// リソース作成
	virtual void CreateVertexBufferResource(ID3D12Device* device, const UINT& numElements, bool isSkinningVertex) = 0;
	// View作成
	virtual bool CreateVBV() = 0;
	// SRV作成
	virtual bool CreateSRV(ID3D12Device8* device, D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, DescriptorHeap* pDescriptorHeap) = 0;
	// UAV作成
	virtual bool CreateUAV(ID3D12Device8* device, D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, DescriptorHeap* pDescriptorHeap, bool useCounter = false) = 0;
	// 頂点バッファビューを取得
	virtual D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() = 0;
};

template<typename T>
class VertexBuffer : public IVertexBuffer
{
public:
	// Constructor
	VertexBuffer() : IVertexBuffer()
	{
	}
	// Constructor
	VertexBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
		IVertexBuffer(pResource, CurrentState)
	{
	}
	// Destructor
	~VertexBuffer()
	{
		m_View = {};
		m_MappedData = std::span<T>{};
	}
	// リソースを作成
	void CreateVertexBufferResource(ID3D12Device* device, const UINT& numElements,bool isSkinningVertex) override
	{
		D3D12_HEAP_TYPE heapType = isSkinningVertex ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_STATES resourceState = isSkinningVertex ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : D3D12_RESOURCE_STATE_GENERIC_READ;
		D3D12_RESOURCE_FLAGS resourceFlag = isSkinningVertex ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		// リソースのサイズ
		UINT structureByteStride = static_cast<UINT>(sizeof(T));
		// リソース用のヒープの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = heapType;
		GpuBuffer::CreateBuffer(
			device, heapProperties, D3D12_HEAP_FLAG_NONE,
			resourceState,
			resourceFlag,
			numElements, structureByteStride);
		if (!isSkinningVertex)
		{
			// マッピング
			T* mappedData = nullptr;// 一時マップ用
			GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
			// マップしたデータをspanに変換
			m_MappedData = std::span<T>(mappedData, static_cast<size_t>(numElements));
		}
	}
	bool CreateVBV() override
	{
		// Viewの作成
		// リソースがあるかどうか確認
		if (!GetResource())
		{
			Log::Write(LogLevel::Assert, "Resource is null");
			return false;
		}
		// Resourceの先頭のアドレスから使う
		m_View.BufferLocation = GetResource()->GetGPUVirtualAddress();
		// 使用するResourceのサイズ
		m_View.SizeInBytes = static_cast<UINT>(GetBufferSize());
		// 要素一つ分のサイズ
		m_View.StrideInBytes = GetStructureByteStride();
		return true;
	}
	// スキニング用SRV
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
	// スキニング用UAV
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
	void UpdateData(const T& data,const uint32_t& index)
	{
		if (index < m_MappedData.size())
		{
			m_MappedData[index] = data;
		} else
		{
			Log::Write(LogLevel::Assert, "Index out of range");
		}
	}
	void MappedDataCopy(std::vector<T>& data)
	{
		if (!m_MappedData.empty())
		{
			std::memcpy(m_MappedData.data(), data.data(), sizeof(T) * GetNumElements());
		} else
		{
			Log::Write(LogLevel::Assert, "MappedData is null");
		}
	}
	// 頂点バッファビューを取得
	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() override
	{
		return &m_View;
	}
private:
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW m_View = {};
	// マップ用データにコピーするためのポインタ
	std::span<T> m_MappedData;
};

