#pragma once
#include "GameCore/SceneManager/SceneManager.h"
#include "GameCore/ECS/ECSManager.h"
#include "GameCore/GameWorld/GameWorld.h"
#include "Core/Utility/CompBufferData.h"

// PhysicsEngine
#include <2D/d2_common.h>
#include <3D/d3_common.h>

class InputManager;
class ResourceManager;
class GraphicsEngine;
class EngineCommand;

// ECSのイベントリスナー
class ComponentEventDispatcher : public IComponentEventListener
{
public:
	ComponentEventDispatcher(EngineCommand* engineCommand) : m_EngineCommand(engineCommand) {}
private:
	EngineCommand* m_EngineCommand = nullptr;
};

class GameCore
{
	friend class TheatriaEngine;
public:
	GameCore()
	{
	}
	~GameCore() 
	{
	}
	void Initialize(ResourceManager* resourceManager, GraphicsEngine* graphicsEngine);
	void SetEngineCommandPtr(EngineCommand* engineCommand) { m_EngineCommand = engineCommand; }
	void Start();
	void Update();
    void Finalize();
	bool IsRunning() const { return isRunning; }
	void GameRun(const bool& isDebugger = false,const bool&isRuntime = false);
	void GameStop();
	physics::d2::Id2World* GetPhysicsWorld2D() { return m_pPy2dWorld.get(); }
	physics::d3::Id3World* GetPhysicsWorld3D() { return m_pPy3dWorld.get(); }
	// 環境設定更新
	void UpdateEnvironmentSetting();

    void SetScriptSystemEngineCommand();

	GameWorld* GetGameWorld() { return m_pGameWorld.get(); }
	SceneManager* GetSceneManager() { return m_pSceneManager.get(); }
	ECSManager* GetECSManager() { return m_pECSManager.get(); }
private:
	void RegisterECSEvents();
	void RegisterECSSystems(ResourceManager* resourceManager, GraphicsEngine* graphicsEngine);
	void RegisterContactEvents();
	
	EngineCommand* m_EngineCommand = nullptr;

	// シーンマネージャー
	std::unique_ptr<SceneManager> m_pSceneManager = nullptr;
	// ECSマネージャ
	std::unique_ptr<ECSManager> m_pECSManager = nullptr;
	// ゲームワールド
	std::unique_ptr<GameWorld> m_pGameWorld = nullptr;
	// ゲーム実行フラグ
	bool isRunning = false;
	// 環境情報
	BUFFER_DATA_ENVIRONMENT m_EnvironmentData;
	// 2dPhysicsワールド
	std::unique_ptr<physics::d2::Id2World> m_pPy2dWorld = nullptr;
	// 3dPhysicsワールド
	std::unique_ptr<physics::d3::Id3World> m_pPy3dWorld = nullptr;
	// ECSイベントディスパッチャー
	std::shared_ptr<ComponentEventDispatcher> m_pComponentEventDispatcher = nullptr;
};

