#include "pch.h"
#include "GpuBuffer.h"

void GpuBuffer::CreateBuffer(ID3D12Device* device, D3D12_HEAP_PROPERTIES& heapProperties, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_STATES InitialState, D3D12_RESOURCE_FLAGS resourceFlags, const UINT& numElements, const UINT& structureByteStride)
{
	// バッファのサイズを取得
	m_BufferSize = static_cast<UINT64>(numElements * structureByteStride);
	// 要素数を取得
	m_NumElements = numElements;
	// 要素のサイズを取得
	m_StructureByteStride = structureByteStride;
	// リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = m_BufferSize;// リソースのサイズ
	// バッファリソースの設定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Flags = resourceFlags;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	GpuResource::CreateResource(device, heapProperties, heapFlags, resourceDesc, InitialState, nullptr);
}
