#pragma once
#include "GameCore/GameObject/GameObject.h"
#include "GameCore/CPrefab/CPrefab.h"
#include "Core/Utility/FVector.h"

// 前方宣言
class SceneManager;

class GameScene
{
public:
	// Constructor
	GameScene(const std::wstring& sceneName) : m_SceneName(sceneName) {}
	// Destructor
	~GameScene() = default;

	void AddPrefab(CPrefab& prefab);
	void RemovePrefab(const std::wstring& prefabName);
	void RemovePrefab(const size_t idx);
	void ClearPrefabs() { m_Prefabs.clear(); m_PrefabNameToIdxMap.clear(); }
	void SetSceneName(const std::wstring& sceneName) { m_SceneName = sceneName; }
	void SetStartCameraName(const std::wstring& cameraName) { m_StartCameraName = cameraName; }
	std::wstring GetName() const { return m_SceneName; }
	std::wstring GetStartCameraName() const { return m_StartCameraName; }
	FVector<CPrefab> GetPrefabs() const { return m_Prefabs; }
	FVector<CPrefab>& GetPrefabs() { return m_Prefabs; }
	std::unordered_map<std::wstring, size_t> GetPrefabNameToIdxMap() const { return m_PrefabNameToIdxMap; }
private:
	std::wstring m_SceneName = L"";
	FVector<CPrefab> m_Prefabs;
	std::unordered_map<std::wstring, size_t> m_PrefabNameToIdxMap;
	std::wstring m_StartCameraName = L"";
};

// シーンのインスタンス
class GameSceneInstance
{
public:
	GameSceneInstance(SceneManager* sm,const SceneID& instanceID) : m_pSceneManager(sm),m_InstanceID(instanceID){}
	~GameSceneInstance() = default;

	void UnloadScene()
	{

	}
private:
	SceneManager* m_pSceneManager = nullptr;
	SceneID m_InstanceID;
};

