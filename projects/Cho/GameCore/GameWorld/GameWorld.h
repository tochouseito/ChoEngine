#pragma once
#include "GameCore/GameObject/GameObject.h"
#include "Core/Utility/FVector.h"

// 前方宣言
class ECSManager;
class SceneManager;
class GameScene;
class GameCore;
// コンテナのエイリアス
using WorldContainer = FVector<FVector<FVector<std::unique_ptr<GameObject>>>>;

class GameWorld
{
	friend class GameCore;
	friend class SceneManager;
public:
	// Constructor
	GameWorld(GameCore* gamecore, ECSManager* ecs) : m_pGameCore(gamecore), m_pECSManager(ecs)
	{
	}
	// Destructor
	~GameWorld()
	{
	}
	void Initialize();
	void Update();
	// オブジェクトを取得
	GameObject* GetGameObject(const std::wstring& name);
	GameObject* GetGameObject(const Entity& e);
	GameObject* GetGameObject(const ObjectHandle& handle);
	// オブジェクトコンテナを取得
	WorldContainer& GetWorldContainer() { return m_pGameObjects; }
	const WorldContainer& GetWorldContainer() const { return m_pGameObjects; }
	// メインカメラを取得
	GameObject* GetMainCamera() { return m_pMainCamera; }
	// メインカメラを設定
	void SetMainCamera(GameObject* pCamera) { m_pMainCamera = pCamera; }
	// メインカメラを空にする
	void ClearMainCamera() { m_pMainCamera = nullptr; }
	// オブジェクトを作成
	ObjectHandle CreateGameObject(const std::wstring& name, ObjectType type);
	// オブジェクトを削除
	void RemoveGameObject(const ObjectHandle& handle);
	// 名前変更
	void RenameGameObject(const ObjectHandle& handle, const std::wstring& newName);
	// クローンを追加
	ObjectHandle AddGameObjectClone(const ObjectHandle& src);
	// コピーを追加
	ObjectHandle CreateGameObjectCopy(const ObjectHandle& src);
	// ワールドからGameSceneを生成
	GameScene CreateGameSceneFromWorld(SceneManager& sceneManager, const std::wstring& editSceneName) const;
private:
	// シーンデータからオブジェクトを作成
	SceneID AddGameObjectFromScene(const GameScene& scene,const bool& updateMaincamera = false);
	// 全シーン破棄
	void ClearAllScenes();
	// タイプごとの初期コンポーネントを追加
	void AddDefaultComponentsToGameObject(ObjectHandle handle, ObjectType type);

	// 削除実行関数
	void RemoveGameObjectImpl(const ObjectHandle& handle);
	void AllClearSceneImpl();
	void Defer(std::function<void()> cmd)
	{
		m_DeferredCommands.push_back(std::move(cmd));
	}
	
	// フレーム末にまとめてコマンドを実行
	void FlushDeferred()
	{
		for (auto& cmd : m_DeferredCommands) cmd();
		m_DeferredCommands.clear();
	}

	GameCore* m_pGameCore = nullptr; // GameCoreへのポインタ
	ECSManager* m_pECSManager = nullptr;	
	// GameObjectコンテナ
	// [SceneID][ObjectID][CloneID] = GameObject
	WorldContainer m_pGameObjects;
	// 名前検索辞書
	std::unordered_map<std::wstring, ObjectHandle> m_ObjectHandleMap;
	// Entityから逆引きする辞書
	std::unordered_map<Entity, ObjectHandle> m_ObjectHandleMapFromEntity;
	// メインカメラ
	GameObject* m_pMainCamera = nullptr;
	std::vector<std::function<void()>>                          m_DeferredCommands;
};

