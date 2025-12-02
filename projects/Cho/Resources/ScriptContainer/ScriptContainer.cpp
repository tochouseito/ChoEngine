#include "pch.h"
#include "ScriptContainer.h"

void ScriptContainer::RegisterScript(const std::string& scriptName, ScriptFactoryFunc func)
{
	m_ScriptRegistry[scriptName] = std::move(func);
}

std::unique_ptr<Marionnette> ScriptContainer::CreateScript(const std::string& scriptName)
{
	auto it = m_ScriptRegistry.find(scriptName);
	if (it != m_ScriptRegistry.end())
	{
		return it->second();// スクリプトのインスタンスを生成
	}
	return nullptr; // スクリプトが見つからない場合はnullptrを返す
}
