#pragma once
#include "Core/Utility/FVector.h"
#include "Core/Utility/CompBufferData.h"
#include <list>
#include <optional>
class ResourceManager;
// 統合バッファのインタフェース
class IIntegrationData
{
public:
	// Constructor
	IIntegrationData(std::optional<uint32_t>& index,const size_t& size)
		: m_BufferIndex(index), m_Size(size)// 初期サイズ
	{
		
	}
	// Destructor
	virtual ~IIntegrationData() = default;
	// サイズを取得
	virtual size_t size() const
	{
		return m_Size;
	}
	// バッファのインデックスを取得
	virtual std::optional<uint32_t> GetBufferIndex() const
	{
		return m_BufferIndex;
	}
	// MapIDを取得
	virtual uint32_t GetMapID()
	{
		// 有効数追加
		m_ActiveCount++;
		// 返却されたIDがあるなら取得
		if (!m_RemoveMapID.empty())
		{
			uint32_t id = m_RemoveMapID.back();
			m_RemoveMapID.pop_back();
			return id;
		}
		// IDを取得
		uint32_t id = m_NextMapID;
		// 一つ進める
		m_NextMapID++;
		return id;
	}
	// MapIDを返却
	virtual void RemoveMapID(const uint32_t& id)
	{
		// 返却されたIDを保存
		m_RemoveMapID.push_back(id);
		// 有効数を減らす
		m_ActiveCount--;
	}
	// 現在のIDを取得
	virtual uint32_t GetCurrentMapID()
	{
		return m_NextMapID;
	}
	// 有効数を取得
	virtual uint32_t GetActiveCount()
	{
		return m_ActiveCount;
	}
protected:
	// リソースマネージャーのポインタ
	ResourceManager* m_ResourceManager = nullptr;
	// バッファのインデックス
	std::optional<uint32_t> m_BufferIndex = std::nullopt;
	// 統合データのサイズ
	size_t m_Size = 0;
	// 統合データの次のID
	uint32_t m_NextMapID = 0;
	// 統合データの返却されたID
	std::vector<uint32_t> m_RemoveMapID = {};
	// 統合データの有効数
	uint32_t m_ActiveCount = 0;
	// 統合データのサイズオフセット
	uint32_t m_SizeOffset = 64;
};

class ResourceManager;
template<typename T>
class IntegrationData : public IIntegrationData
{
public:
	// Constructor
	IntegrationData(std::optional<uint32_t>& index,const size_t& size)
		: IIntegrationData(index,size)
	{
		
	}
	// Destructor
	~IntegrationData()
	{

	}
	
private:

};