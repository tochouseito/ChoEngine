#include "pch.h"
#include "GameScene.h"
#include <algorithm>

void GameScene::AddPrefab(CPrefab& prefab)
{
	if (prefab.GetName().empty())
	{
		throw std::invalid_argument("Prefab name cannot be empty.");
	}
	m_PrefabNameToIdxMap[prefab.GetName()] = m_Prefabs.push_back(std::move(prefab));
}

void GameScene::RemovePrefab(const std::wstring& prefabName)
{
	if (m_PrefabNameToIdxMap.contains(prefabName))
	{
		m_Prefabs.erase(m_PrefabNameToIdxMap[prefabName]);
		m_PrefabNameToIdxMap.erase(prefabName);
	}
}

void GameScene::RemovePrefab(const size_t idx)
{
	if (m_Prefabs.isValid(idx))
	{
		m_Prefabs.erase(idx);
	}
}
