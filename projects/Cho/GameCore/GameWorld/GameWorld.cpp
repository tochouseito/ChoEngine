#include "pch.h"
#include "GameWorld.h"
#include "GameCore/GameCore.h"
#include "GameCore/GameScene/GameScene.h"
#include "Core/Utility/GenerateUnique.h"
#include "Core/ChoLog/ChoLog.h"
#include "GameCore/SceneManager/SceneManager.h"
#include "GameCore/ECS/ECSManager.h"

void GameWorld::Initialize()
{

}

void GameWorld::Update()
{

} 

// オブジェクトを取得
GameObject* GameWorld::GetGameObject(const std::wstring& name)
{
	if (!m_ObjectHandleMap.contains(name))
	{
		return nullptr;
	}
	ObjectHandle handle = m_ObjectHandleMap[name];
	return m_pGameObjects[handle.sceneID][handle.objectID][handle.cloneID].get();
}

GameObject* GameWorld::GetGameObject(const Entity& e)
{
	if (!m_ObjectHandleMapFromEntity.contains(e))
	{
		return nullptr;
	}
	ObjectHandle handle = m_ObjectHandleMapFromEntity[e];
	return m_pGameObjects[handle.sceneID][handle.objectID][handle.cloneID].get();
}

GameObject* GameWorld::GetGameObject(const ObjectHandle& handle)
{
	if (!m_pGameObjects.isValid(handle.sceneID) ||
		!m_pGameObjects[handle.sceneID].isValid(handle.objectID) ||
		!m_pGameObjects[handle.sceneID][handle.objectID].isValid(handle.cloneID))
	{
		return nullptr;
	}
	return m_pGameObjects[handle.sceneID][handle.objectID][handle.cloneID].get();
}

// オブジェクトを作成
ObjectHandle GameWorld::CreateGameObject(const std::wstring& name, ObjectType type)
{
	// handleを作成
	ObjectHandle handle;
	handle.sceneID = 0; // 0番のSceneに追加
	// Entityを取得
	Entity entity = m_pECSManager->GenerateEntity();
	handle.entity = entity;
	// 基本コンポーネントを追加
	m_pECSManager->AddComponent<TransformComponent>(handle.entity);
	theatriaSystem::Transform transform(entity, m_pECSManager);
	// オブジェクトIDを取得
	uint32_t objectID = static_cast<uint32_t>(m_pGameObjects[0].push_back(FVector<std::unique_ptr<GameObject>>()));
	handle.objectID = objectID;
	// 名前被り防止
	std::wstring newName = GenerateUniqueName<std::unordered_map<std::wstring, ObjectHandle>>(name,m_ObjectHandleMap);
	// GameObjectを作成
	m_pGameObjects[0][objectID].push_back(std::make_unique<GameObject>(m_pECSManager,handle, newName, type,transform));
	// Entityにコンポーネントを追加
	AddDefaultComponentsToGameObject(handle, type);
	// 辞書に登録
	m_ObjectHandleMap[newName] = handle;
	m_ObjectHandleMapFromEntity[entity] = handle;
	return handle;
}

// シーンデータからオブジェクトを作成
SceneID GameWorld::AddGameObjectFromScene(const GameScene& scene, const bool& updateMaincamera)
{
	// シーンIDを取得
	SceneID sceneID = static_cast<SceneID>(m_pGameObjects.push_back(FVector<FVector<std::unique_ptr<GameObject>>>()));
	// シーンのオブジェクトを作成
	FVector<CPrefab> prefabs = scene.GetPrefabs();
	for (auto& prefab : prefabs)
	{
		// handleを作成
		ObjectHandle handle;
		handle.sceneID = sceneID; // シーンIDを設定
		// PrefabからEntityを生成
		Entity entity = prefab.Instantiate(*m_pECSManager);
		handle.entity = entity;
		MeshFilterComponent* meshFilter = m_pECSManager->GetComponent<MeshFilterComponent>(entity);
		meshFilter;
		// 基本コンポーネントを追加
		// m_pECSManager->AddComponent<TransformComponent>(handle.entity);
		theatriaSystem::Transform transform(entity, m_pECSManager);
		// オブジェクトIDを取得
		uint32_t objectID = static_cast<uint32_t>(m_pGameObjects[sceneID].push_back(FVector<std::unique_ptr<GameObject>>()));
		handle.objectID = objectID;
		// GameObjectを作成
		if (prefab.GetName().empty())
		{
			theatria::Log::Write(theatria::LogLevel::Assert, "Prefab name is empty. Please set a valid name for the prefab.");
			return sceneID;
		}
		if(prefab.GetType() == ObjectType::None)
		{
			theatria::Log::Write(theatria::LogLevel::Assert, "Prefab type is None. Please set a valid type for the prefab.");
			return sceneID;
		}
		m_pGameObjects[sceneID][objectID].push_back(std::make_unique<GameObject>(m_pECSManager,handle, prefab.GetName(), prefab.GetType(),transform));
		// 辞書に登録
		m_ObjectHandleMap[prefab.GetName()] = handle;
		m_ObjectHandleMapFromEntity[entity] = handle;
	}
	// シーンのカメラを設定
	if (updateMaincamera)
	{
		// メインカメラを取得
		GameObject* cameraObject = GetGameObject(scene.GetStartCameraName());
		SetMainCamera(cameraObject);
	}
	return sceneID;
}

// ワールドからGameSceneを生成
GameScene GameWorld::CreateGameSceneFromWorld(SceneManager& sceneManager, const std::wstring& editSceneName) const
{
	// 編集対象のシーンをGameSceneに変換
	// とりあえず0番目のシーンを対象にする
	// クローンは除外
	std::wstring sceneName = sceneManager.GetScene(editSceneName)->GetName();
	GameScene gameScene(sceneName);
	for (auto& object : m_pGameObjects[0])
	{
		// オブジェクトのエンティティからCPrefabを生成
		CPrefab prefab = CPrefab::FromEntity(*m_pECSManager,
			object[0]->GetHandle().entity,
			object[0]->GetName(),
			object[0]->GetType());
		gameScene.AddPrefab(prefab);
	}
	// シーンのカメラを設定
	if (m_pMainCamera)
	{
		gameScene.SetStartCameraName(m_pMainCamera->GetName());
	}
	else
	{
		gameScene.SetStartCameraName(L"None");
	}

	return gameScene;
}


// オブジェクトを削除
void GameWorld::RemoveGameObject(const ObjectHandle& handle)
{
	// ECSから削除
	m_pECSManager->RemoveEntity(handle.entity);
	// ゲーム更新中なら遅延キュー
	if (m_pGameCore->IsRunning())
	{
		Defer([this, handle]() { RemoveGameObjectImpl(handle); } );
	}
	else
	{
		// ゲーム更新中でなければ即時削除
		RemoveGameObjectImpl(handle);
	}
}

// 名前変更
void GameWorld::RenameGameObject(const ObjectHandle& handle, const std::wstring& newName)
{
	// GameObjectを取得
	GameObject* gameObject = GetGameObject(handle);
	// 同じ名前なら何もしない
	if (gameObject->GetName() == newName) { return; }
	// 名前被り防止
	std::wstring uniqueName = GenerateUniqueName<std::unordered_map<std::wstring, ObjectHandle>>(newName, m_ObjectHandleMap);
	// 辞書から削除
	m_ObjectHandleMap.erase(gameObject->GetName());
	// 辞書に新しい名前で登録
	m_ObjectHandleMap[uniqueName] = handle;
	// 名前を変更
	gameObject->SetName(uniqueName);
}


// クローンを追加
ObjectHandle GameWorld::AddGameObjectClone(const ObjectHandle& src)
{
	// srcのGameObjectを取得
	GameObject* srcGameObject = GetGameObject(src);
	// handleを作成
	ObjectHandle handle;
	// クローンフラグ
	handle.isClone = true;
	handle.sceneID = src.sceneID; // srcのSceneに追加
	// クローン元のオブジェクトIDを設定
	handle.originalID = src.objectID;
	// Entityを取得
	Entity entity = m_pECSManager->GenerateEntity();
	//Entity entity = m_pECSManager->CopyEntity(src.entity);
	handle.entity = entity;
	// 基本コンポーネントを追加
	// m_pECSManager->AddComponent<TransformComponent>(handle.entity);
	theatriaSystem::Transform transform(entity, m_pECSManager);
	// objectIDを取得
	handle.objectID = src.objectID; // srcのobjectIDを使用
	// 名前被り防止
	std::wstring newName = GenerateUniqueName<std::unordered_map<std::wstring, ObjectHandle>>(srcGameObject->GetName(), m_ObjectHandleMap);
	// srcのクローンリストに追加
	handle.cloneID = static_cast<uint32_t>(m_pGameObjects[src.sceneID][src.objectID].push_back(std::make_unique<GameObject>(m_pECSManager,handle, newName, srcGameObject->GetType(),transform)));
	GameObject* dstGameObject = m_pGameObjects[src.sceneID][src.objectID][handle.cloneID].get();
	dstGameObject->SetHandle(handle); // クローンIDを設定
	// Entityにコンポーネントを追加
	// AddDefaultComponentsToGameObject(handle, srcGameObject->GetType());
	// 辞書に登録
	m_ObjectHandleMap[newName] = handle;
	m_ObjectHandleMapFromEntity[entity] = handle;
	// クローン元のEntityをコピー
	m_pECSManager->CopyEntity(src.entity, entity);
	return handle;
}

ObjectHandle GameWorld::CreateGameObjectCopy(const ObjectHandle& src)
{
	// srcのGameObjectを取得
	GameObject* srcGameObject = GetGameObject(src);
	// handleを作成
	ObjectHandle handle;
	handle.sceneID = 0; 
	// Entityを取得
	Entity entity = m_pECSManager->CopyEntity(src.entity);
	handle.entity = entity;
	
	theatriaSystem::Transform transform(entity, m_pECSManager);
	// オブジェクトIDを取得
	uint32_t objectID = static_cast<uint32_t>(m_pGameObjects[0].push_back(FVector<std::unique_ptr<GameObject>>()));
	handle.objectID = objectID;
	// 名前被り防止
	std::wstring newName = GenerateUniqueName<std::unordered_map<std::wstring, ObjectHandle>>(srcGameObject->GetName(), m_ObjectHandleMap);
	// GameObjectを作成
	m_pGameObjects[0][objectID].push_back(std::make_unique<GameObject>(m_pECSManager,handle, newName, srcGameObject->GetType(),transform));
	// 辞書に登録
	m_ObjectHandleMap[newName] = handle;
	m_ObjectHandleMapFromEntity[entity] = handle;
	return handle;
}

// 全シーン破棄
void GameWorld::ClearAllScenes()
{
	//for (auto& scene : m_pGameObjects)
	//{
	//	for (auto& object : scene)
	//	{
	//		for (auto& clone : object)
	//		{
	//			//std::wstring name = clone->GetName();
	//			//Entity entity = clone->GetHandle().entity;
	//			//// ECSから削除
	//			//m_pECSManager->RemoveEntity(entity);
	//			//RemoveGameObjectImpl(clone->GetHandle());
	//		}
	//	}
	//}
	//// コンテナと辞書をクリア
	//m_pGameObjects.clear();
	//m_ObjectHandleMap.clear();
	//m_ObjectHandleMapFromEntity.clear();
	//// メインカメラをクリア
	//m_pMainCamera = nullptr;
	for (auto& scene : m_pGameObjects)
	{
		for (auto& object : scene)
		{
			for (auto& clone : object)
			{
				// ECSから削除
				m_pECSManager->RemoveEntity(clone->GetHandle().entity);
				// RemoveGameObject(clone->GetHandle());
			}
		}
	}
	AllClearSceneImpl();
	m_pECSManager->CancelUpdateLoop();
}

// タイプごとの初期コンポーネントを追加
void GameWorld::AddDefaultComponentsToGameObject(ObjectHandle handle, ObjectType type)
{
	switch (type)
	{
	case ObjectType::MeshObject:
		m_pECSManager->AddComponent<MeshFilterComponent>(handle.entity);
		m_pECSManager->AddComponent<MeshRendererComponent>(handle.entity);
		break;
	case ObjectType::Camera:
		m_pECSManager->AddComponent<CameraComponent>(handle.entity);
		break;
	case ObjectType::ParticleSystem:
		m_pECSManager->AddComponent<MeshFilterComponent>(handle.entity);
		m_pECSManager->AddComponent<MeshRendererComponent>(handle.entity);
		m_pECSManager->AddComponent<MaterialComponent>(handle.entity);
		m_pECSManager->AddComponent<ParticleComponent>(handle.entity);
		m_pECSManager->AddComponent<EmitterComponent>(handle.entity);
		break;
	case ObjectType::Effect:
		break;
	case ObjectType::Light:
		break;
	case ObjectType::UI:
		m_pECSManager->AddComponent<UISpriteComponent>(handle.entity);
		m_pECSManager->AddComponent<MaterialComponent>(handle.entity);
		break;
	case ObjectType::None:
		break;
	case ObjectType::Count:
		break;
	default:
		break;
	}
}

// 削除実行関数
void GameWorld::RemoveGameObjectImpl(const ObjectHandle& handle)
{
	// GameObjectを取得
	GameObject* gameObject = GetGameObject(handle);
	if (!gameObject) return; // 存在しない場合は何もしない
	// 辞書から削除
	m_ObjectHandleMap.erase(gameObject->GetName());
	m_ObjectHandleMapFromEntity.erase(handle.entity);
	// コンテナから削除
	// クローンならクローンリストから削除
	if (handle.isClone)
	{
		// クローンリストから削除
		m_pGameObjects[handle.sceneID][handle.objectID].erase(handle.cloneID);
	}
	else // クローンでないならオブジェクトリストから削除
	{
		m_pGameObjects[handle.sceneID].erase(handle.objectID);
	}
}

void GameWorld::AllClearSceneImpl()
{
	// コンテナと辞書をクリア
	m_pGameObjects.clear();
	m_ObjectHandleMap.clear();
	m_ObjectHandleMapFromEntity.clear();
	// メインカメラをクリア
	m_pMainCamera = nullptr;
}
