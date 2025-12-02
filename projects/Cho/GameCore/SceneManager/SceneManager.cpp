#include "pch.h"
#include "SceneManager.h"
#include "GameCore/GameWorld/GameWorld.h"
#include "Core/Utility/GenerateUnique.h"

// デフォルトのシーンを作成
GameScene SceneManager::CreateDefaultScene(const std::wstring& name)
{
	// シーン名を生成
	std::wstring sceneName = GenerateUniqueName<std::unordered_map<std::wstring, SceneID>>(name, m_SceneNameToID);
	GameScene scene(sceneName);
	// デフォルトのObjectを追加
	// Cube
	CPrefab cube(L"Cube", ObjectType::MeshObject);
	TransformComponent transform;
	MeshFilterComponent meshFilter;
	meshFilter.modelName = L"Cube"; // モデル名を設定
	MeshRendererComponent meshRenderer;
	cube.AddComponent<TransformComponent>(transform);
	cube.AddComponent<MeshFilterComponent>(meshFilter);
	cube.AddComponent<MeshRendererComponent>(meshRenderer);
	// MainCamera
	CPrefab mainCamera(L"MainCamera", ObjectType::Camera);
	TransformComponent cameraTransform;
	cameraTransform.position.z = -20.0f;
	CameraComponent cameraComponent;
	mainCamera.AddComponent<TransformComponent>(cameraTransform);
	mainCamera.AddComponent<CameraComponent>(cameraComponent);
	// DirectionalLight
	CPrefab directionalLight(L"DirectionalLight", ObjectType::Light);
	TransformComponent lightTransform;
	lightTransform.degrees.x = 45.0f;
	lightTransform.degrees.y = 45.0f;
	LightComponent lightComponent;
	lightComponent.type = LightType::Directional;
	directionalLight.AddComponent<TransformComponent>(lightTransform);
	directionalLight.AddComponent<LightComponent>(lightComponent);
	// シーンに追加
	scene.AddPrefab(cube);
	scene.AddPrefab(mainCamera);
	scene.AddPrefab(directionalLight);
	// mainCameraを設定
	scene.SetStartCameraName(L"MainCamera");
	AddScene(scene);
	return m_Scenes[m_SceneNameToID[sceneName]]; // シーンを返す
}

// 読み込まれているシーンを破棄して指定したシーンをロード
GameSceneInstance* SceneManager::LoadScene(const std::wstring& sceneName, const bool& updateMaincamera)
{
	// コンテナ
	if (!m_SceneNameToID.contains(sceneName))
	{
		return nullptr; // シーンが存在しない場合はnullptrを返す
	}
	// 読み込まれているシーンをすべて破棄
	m_pGameWorld->ClearAllScenes(); // ゲームワールドのシーンをクリア
	// シーンインスタンスを破棄
	m_pSceneInstances.clear();
	// ロード
	SceneID id = m_pGameWorld->AddGameObjectFromScene(m_Scenes[m_SceneNameToID[sceneName]],updateMaincamera);
	// シーンインスタンスを作成
	m_pSceneInstances.push_back(std::make_unique<GameSceneInstance>(this, id));
	// シーンインスタンスを返す
	return m_pSceneInstances.back().get();
}

GameSceneInstance* SceneManager::LoadTemporaryScene(const GameScene& scene, const bool& updateMaincamera)
{
	// 読み込まれているシーンをすべて破棄
	m_pGameWorld->ClearAllScenes(); // ゲームワールドのシーンをクリア
	// シーンインスタンスを破棄
	m_pSceneInstances.clear();
	// ロード
	SceneID id = m_pGameWorld->AddGameObjectFromScene(scene, updateMaincamera);
	// シーンインスタンスを作成
	m_pSceneInstances.push_back(std::make_unique<GameSceneInstance>(this, id));
	// シーンインスタンスを返す
	return m_pSceneInstances.back().get();
}

// シーンをアンロード
void SceneManager::UnLoadScene(GameSceneInstance* pSceneInstance)
{
	pSceneInstance->UnloadScene(); // シーンインスタンスのアンロード
}

// シーンを追加
void SceneManager::AddScene(GameScene scene)
{
	if (m_SceneNameToID.contains(scene.GetName()))
	{
		return; // 既に存在する場合は何もしない
	}
	std::wstring sceneName = scene.GetName();
	SceneID id = static_cast<SceneID>(m_Scenes.push_back(std::move(scene)));
	m_SceneNameToID[sceneName] = id; // シーン名とIDを紐付け
}

// コンテナからシーンを取得
GameScene* SceneManager::GetScene(const SceneID& sceneID) noexcept
{
	if (m_Scenes.isValid(sceneID))
	{
		return &m_Scenes[sceneID];
	}
	return nullptr;
}

GameScene* SceneManager::GetScene(const std::wstring& sceneName) noexcept
{
	if (m_SceneNameToID.contains(sceneName))
	{
		return &m_Scenes[m_SceneNameToID.at(sceneName)];
	}
	return nullptr;
}

// シーン名からシーンIDを取得
SceneID SceneManager::GetSceneID(const std::wstring& sceneName) const noexcept
{
	if (m_SceneNameToID.contains(sceneName))
	{
		return m_SceneNameToID.at(sceneName);
	}
	return 0;
}

// シーン名からシーンを取得
GameScene* SceneManager::GetSceneToName(const std::wstring& sceneName) noexcept
{
	if (m_SceneNameToID.contains(sceneName))
	{
		return &m_Scenes[m_SceneNameToID.at(sceneName)];
	}
	return nullptr;
}
