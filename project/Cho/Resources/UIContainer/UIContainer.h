#pragma once
#include "ChoMath.h"
#include "Core/Utility/FVector.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <list>

struct UIVertexData
{
	float4 position;
	float2 texCoord;
	float4 color;
	uint32_t vertexID;
};

struct UIData
{
	std::vector<UIVertexData> vertices;
	std::vector<uint32_t> indices;
	std::optional<uint32_t> vertexBufferIndex = std::nullopt;
	std::optional<uint32_t> indexBufferIndex = std::nullopt;
};

class ResourceManager;
class UIContainer
{
public:
	UIContainer(ResourceManager* resourceManager);
	~UIContainer()
	{

	}
	// UIDataの追加
	uint32_t AddUIData();
	// UIの追加
	void AddUI(const uint32_t& mapID)
	{
		m_UseList.push_back(mapID);
		UpdateUseListBuffer();
	}
	// UIDataの削除
	void RemoveUI(const uint32_t& mapID)
	{
		m_UseList.remove(mapID);
		UpdateUseListBuffer();
	}
	// UIDataの取得
	UIData& GetUIData(uint32_t index)
	{
		return m_UIDataContainer[index];
	}
	// UseListの取得
	std::list<uint32_t>& GetUseList()
	{
		return m_UseList;
	}
	// UseListBufferIndexの取得
	uint32_t GetUseListBufferIndex()
	{
		return m_UseListBufferIndex;
	}
	void UpdateUseListBuffer();
private:

	// UIDataのコンテナ
	FVector<UIData> m_UIDataContainer;
	// UseList
	std::list<uint32_t> m_UseList;
	// UseListBufferIndex
	uint32_t m_UseListBufferIndex = 0;
	// UseListのオフセット
	const uint32_t kUseListOffset = 128;

	// ResourceManagerのポインタ
	ResourceManager* m_pResourceManager = nullptr;
};

