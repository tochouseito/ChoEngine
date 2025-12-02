#pragma once
#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <memory>
#include <functional>
#include "Core/Utility/FVector.h"
#include "GameCore/Marionnette/Marionnette.h"
using ScriptFactoryFunc = std::function<std::unique_ptr<Marionnette>()>;

struct ScriptData
{
    std::string scriptName; // スクリプト名
    // metadeta
    std::unordered_map<std::string, ScriptComponent::FieldVal> saveFields; // 保存用
};

// スクリプトを格納するコンテナクラス
class ScriptContainer
{
public:
	// Constructor
	ScriptContainer() = default;
	// Destructor
	~ScriptContainer() = default;
	// スクリプトデータを追加する関数
	void AddScriptData(const std::string& scriptName)
	{
		// すでに同じ名前のスクリプトが存在する場合は何もしない
		if (m_ScriptNameToIndex.contains(scriptName))
		{
			return; // 既に存在する場合は何もしない
		}
		std::string name = scriptName;
        ScriptData data;
        data.scriptName = name;
        uint32_t index = static_cast<uint32_t>(m_ScriptDataContainer.push_back(std::move(data))); // スクリプト名をコンテナに追加
		m_ScriptNameToIndex[scriptName] = index; // スクリプト名とインデックスをマップに追加
	}
	// スクリプトデータをIDで取得する関数
	std::optional<ScriptData> GetScriptDataByID(const uint32_t & index)
	{
		if (m_ScriptDataContainer.isValid(index))
		{
			return m_ScriptDataContainer[index];
		} else
		{
			return std::nullopt; // 無効なIDの場合は空のデータを返す
		}
	}
	// スクリプトデータを名前で取得する関数
	ScriptData* GetScriptDataByName(const std::string & scriptName)
	{
		if (m_ScriptNameToIndex.contains(scriptName))
		{
			uint32_t id = m_ScriptNameToIndex[scriptName];
            ScriptData* data = &m_ScriptDataContainer[id];
			return data;
		}
		return nullptr; // 無効な名前の場合は空のデータを返す
	}
	size_t GetScriptCount() { return m_ScriptDataContainer.size(); }
	FVector<ScriptData>& GetScriptNameContainer() { return m_ScriptDataContainer; } // スクリプト名コンテナを取得

	void RegisterScript(const std::string& scriptName, ScriptFactoryFunc func);
	std::unique_ptr<Marionnette> CreateScript(const std::string& scriptName);
private:
	FVector<ScriptData> m_ScriptDataContainer; // スクリプトデータを格納するベクター
	// 名前で検索する用のコンテナ
	std::unordered_map<std::string, uint32_t> m_ScriptNameToIndex; // スクリプト名からスクリプトIDを取得するためのマップ

	// 登録されたスクリプト名と生成関数のマップ
	std::unordered_map<std::string, ScriptFactoryFunc> m_ScriptRegistry;
};

