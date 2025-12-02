#include "pch.h"
#include "UIContainer.h"
#include "Resources/ResourceManager/ResourceManager.h"

UIContainer::UIContainer(ResourceManager* resourceManager)
	:m_pResourceManager(resourceManager)
{
	// UIDataの追加
	AddUIData();
	// UseListBufferの作成
	m_UseListBufferIndex = m_pResourceManager->CreateStructuredBuffer<uint32_t>(kUseListOffset);
}

uint32_t UIContainer::AddUIData()
{
	UIData uiData;

	// 頂点数とインデックス数
	uint32_t vertices = 4;// 頂点数
	uint32_t indices = 6;// インデックス数
	// メモリ確保
	uiData.vertices.resize(vertices);
	uiData.indices.resize(indices);
	// 頂点データを設定
#pragma region
	// 頂点データ（重複なし）
	uiData.vertices[0] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } ,{1.0f,1.0f,1.0f,1.0f} ,{0} };  // 左上
	uiData.vertices[1] = { { 640.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f },{1.0f,1.0f,1.0f,1.0f},{1} };  // 右上
	uiData.vertices[2] = { { 0.0f, 360.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } ,{1.0f,1.0f,1.0f,1.0f},{2} };  // 左下
	uiData.vertices[3] = { { 640.0f, 360.0f, 0.0f, 1.0f }, { 1.0f, 1.0f },{1.0f,1.0f,1.0f,1.0f},{3} };  // 右下
	// インデックスデータ
	uiData.indices[0] = 0;
	uiData.indices[1] = 1;
	uiData.indices[2] = 2;
	uiData.indices[3] = 1;
	uiData.indices[4] = 3;
	uiData.indices[5] = 2;
#pragma endregion

	// VertexBuffer,IndexBufferの作成
	uiData.vertexBufferIndex = m_pResourceManager->CreateVertexBuffer<UIVertexData>(static_cast<UINT>(uiData.vertices.size()));
	uiData.indexBufferIndex = m_pResourceManager->CreateIndexBuffer<uint32_t>(static_cast<UINT>(uiData.indices.size()));
	// VertexBuffer,IndexBufferにデータを転送
	VertexBuffer<UIVertexData>* vertexBuffer = dynamic_cast<VertexBuffer<UIVertexData>*>(m_pResourceManager->GetBuffer<IVertexBuffer>(uiData.vertexBufferIndex));
	IndexBuffer<uint32_t>* indexBuffer = dynamic_cast<IndexBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IIndexBuffer>(uiData.indexBufferIndex));
	// コピー
	vertexBuffer->MappedDataCopy(uiData.vertices);
	indexBuffer->MappedDataCopy(uiData.indices);

	uint32_t index = static_cast<uint32_t>(m_UIDataContainer.push_back(std::move(uiData)));
	return index;
}

void UIContainer::UpdateUseListBuffer()
{
	StructuredBuffer<uint32_t>* useListBuffer = dynamic_cast<StructuredBuffer<uint32_t>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(m_UseListBufferIndex));
	uint32_t i = 0;
	for (uint32_t& index : m_UseList)
	{
		useListBuffer->UpdateData(index, i);
		i++;
	}
}
