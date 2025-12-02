#include "pch.h"
#include "EditorManager.h"
#include "EngineCommand/EngineCommand.h"
#include "GameCore/GameCore.h"

EditorManager::EditorManager(EngineCommand* engineCommand, InputManager* inputManager) :
	m_EngineCommand(engineCommand), m_InputManager(inputManager)
{
	m_Toolbar = std::make_unique<Toolbar>(this);
	m_MainMenu = std::make_unique<MainMenu>(this, m_Toolbar.get());
	m_DebugCamera = std::make_unique<DebugCamera>(this);
	m_Manipulate = std::make_unique<Manipulate>(this, m_DebugCamera.get());
	m_SceneView = std::make_unique<SceneView>(this, m_DebugCamera.get(),m_Manipulate.get());
	m_GameView = std::make_unique<GameView>(this);
	m_Hierarchy = std::make_unique<Hierarchy>(this);
	m_Inspector = std::make_unique<Inspector>(this);
	m_AssetBrowser = std::make_unique<AssetBrowser>(this);
	m_Console = std::make_unique<Console>(this);
	m_EffectEditor = std::make_unique<EffectEditor>(this);
	m_EffectView = std::make_unique<EffectView>(this, m_DebugCamera.get());
	m_EffectHierarchy = std::make_unique<EffectHierarchy>(this);
	m_MaterialEditor = std::make_unique<MaterialEditor>(this);
	engineCommand->SetEditorManager(this);
}

void EditorManager::Initialize(const bool& isRuntime)
{
	// Toolbar
	m_Toolbar->Initialize();
	// MainMenu
	m_MainMenu->Initialize();
	// DebugCamera
	m_DebugCamera->Initialize();
	// Manipulate
	m_Manipulate->Initialize();
	// SceneView
	m_SceneView->Initialize();
	// GameView
	m_GameView->Initialize();
	// Hierarchy
	m_Hierarchy->Initialize();
	// Inspector
	m_Inspector->Initialize();
	// AssetBrowser
	m_AssetBrowser->Initialize();
	// Console
	m_Console->Initialize();
	// EffectView
	m_EffectView->Initialize();
	// EffectEditor
	m_EffectEditor->Initialize();
	// EffectHierarchy
	m_EffectHierarchy->Initialize();
	// MaterialEditor
	m_MaterialEditor->Initialize();
	if(!isRuntime){
		// BuildWatcherと接続
		theatria::FileSystem::ScriptProject::ConnectPipeToBuildWatcher(60000);// 60秒
	}
}

void EditorManager::Update()
{
	// MainMenu
	m_MainMenu->Update();
	// AssetBrowser
	m_AssetBrowser->Update();
	// Console
	m_Console->Update();
	switch (m_WorkSpaceType)
	{
	case SceneEdit:
		// SceneView
		m_SceneView->Update();
		// GameView
		m_GameView->Update();
		// Hierarchy
		m_Hierarchy->Update();
		// Inspector
		m_Inspector->Update();
		// MaterialEditor
		m_MaterialEditor->Update();
		break;
	case EffectEdit:
		// EffectView
		m_EffectView->Update();
		// EffectEditor
		m_EffectEditor->Update();
		// EffectHierarchy
		m_EffectHierarchy->Update();
		break;
	default:
		break;
	}
}

void EditorManager::SetWorkSpaceType(const std::string& typeName)
{
	if (typeName == "SceneEdit") { m_WorkSpaceType = WorkSpaceType::SceneEdit; }
	if (typeName == "EffectEdit") { m_WorkSpaceType = WorkSpaceType::EffectEdit; }
}

GameObject* EditorManager::GetSelectedGameObject() const
{
	return m_EngineCommand->GetGameCore()->GetGameWorld()->GetGameObject(m_SelectedGameObjectName);
}

void EditorManager::SetSelectedGameObject(GameObject* gameObject)
{
	if (gameObject)
	{
		m_SelectedGameObjectName = gameObject->GetName();
	}
	else
	{
		m_SelectedGameObjectName.clear();
	}
}

void EditorManager::ChangeEditingScene(const std::wstring& sceneName)
{
	// 編集中のシーンを保存
	if (!m_EditingSceneName.empty())
	{
		SaveEditingScene();
	}
	// 初めて編集するシーンの時は新たに編集中のシーンを作成する
	if (m_pSceneMap.contains(sceneName))
	{
		m_EditingSceneName = sceneName;
		// scene読み込み
		m_EngineCommand->GetGameCore()->GetSceneManager()->LoadTemporaryScene(m_SceneList[m_pSceneMap[sceneName]],true);
	}
	else
	{
		m_EditingSceneName = sceneName;
		// 元のシーンをコピーする
		GameScene newScene = *m_EngineCommand->GetGameCore()->GetSceneManager()->GetScene(sceneName);
		m_pSceneMap[sceneName] = m_SceneList.push_back(std::move(newScene));
		// scene読み込み
		m_EngineCommand->GetGameCore()->GetSceneManager()->LoadTemporaryScene(m_SceneList[m_pSceneMap[sceneName]],true);
	}
}

void EditorManager::SaveEditingScene()
{
	// GameWorldからシーンを保存
	GameScene scene = m_EngineCommand->GetGameCore()->GetGameWorld()->CreateGameSceneFromWorld(*m_EngineCommand->GetGameCore()->GetSceneManager(),m_EditingSceneName);
	// EditorManagerのシーンマップに保存
	m_SceneList[m_pSceneMap[m_EditingSceneName]] = std::move(scene);
}

void EditorManager::ReloadEditingScene()
{
	// scene読み込み
	m_EngineCommand->GetGameCore()->GetSceneManager()->LoadTemporaryScene(m_SceneList[m_pSceneMap[m_EditingSceneName]], true);
}

// Sceneを取得
GameScene* EditorManager::GetEditScene(const std::wstring& sceneName)
{
	if (m_pSceneMap.contains(sceneName))
	{
		return &m_SceneList[m_pSceneMap[sceneName]];
	}
	return nullptr;
}
