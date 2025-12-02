#pragma once
#include "GameCore/GameScene/GameScene.h"


enum class LoadSceneMode
{
	Single,  // 単一ロード
	Additive,// 追加ロード
};

// 前方宣言
class GameWorld;

class SceneManager
{
public:
	// Constructor
	SceneManager(GameWorld* gameWorld) : m_pGameWorld(gameWorld)
	{
	}
	// Destructor
	~SceneManager()
	{
	}
	// デフォルトのシーンを作成
	GameScene CreateDefaultScene(const std::wstring& name = L"MainScene");
	// 読み込まれているシーンを破棄して指定したシーンをロード
	GameSceneInstance* LoadScene(const std::wstring& sceneName, const bool& updateMaincamera = false);
	// 一時的に作成したシーンをロードする
	GameSceneInstance* LoadTemporaryScene(const GameScene& scene, const bool& updateMaincamera = false);
	// 非同期でシーンをロード
	GameSceneInstance* LoadSceneAsync(const std::wstring& sceneName, const LoadSceneMode& mode)
	{
		sceneName; // 使用しない引数
		mode; // 使用しない引数
	}
	// シーンをアンロード
	void UnLoadScene(GameSceneInstance* pSceneInstance);
	// シーンを追加
	void AddScene(GameScene scene);
	// コンテナからシーンを取得
	GameScene* GetScene(const SceneID& sceneID) noexcept;
	GameScene* GetScene(const std::wstring& sceneName) noexcept;
	// シーンコンテナを取得
	FVector<GameScene>& GetScenes() noexcept { return m_Scenes; }
	// シーン名からシーンIDを取得
	SceneID GetSceneID(const std::wstring& sceneName) const noexcept;
	// シーン名からシーンを取得
	GameScene* GetSceneToName(const std::wstring& sceneName) noexcept;

	void SetLoadingSceneName(const std::wstring& sceneName) { m_LoadingSceneName = sceneName; }
	const std::wstring& GetLoadingSceneName() const { return m_LoadingSceneName; }
private:
	GameWorld* m_pGameWorld = nullptr; // ゲームワールドへのポインタ

	// シーンコンテナ（フリーリスト付き）
	FVector<GameScene> m_Scenes;
	// 名前検索用補助コンテナ
	std::unordered_map<std::wstring, SceneID> m_SceneNameToID;
	// シーンインスタンスコンテナ
	FVector<std::unique_ptr<GameSceneInstance>> m_pSceneInstances;

	// 読み込むシーンの名前
	std::wstring m_LoadingSceneName = L"";
};

