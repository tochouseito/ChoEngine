#include "pch.h"
#include "GameCore.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Platform/FileSystem/FileSystem.h"
#include "GameCore/Systems/SingleSystems.h"
#include "GameCore/Systems/MultiSystems.h"
#include "EngineCommand/EngineCommands.h"
#include "Editor/EditorManager/EditorManager.h"

void GameCore::Initialize(ResourceManager* resourceManager, GraphicsEngine* graphicsEngine)
{
	// ECSマネージャの生成
	m_pECSManager = std::make_unique<ECSManager>();
	// ゲームワールドの生成
	m_pGameWorld = std::make_unique<GameWorld>(this, m_pECSManager.get());
	// シーンマネージャーの生成
	m_pSceneManager = std::make_unique<SceneManager>(m_pGameWorld.get());
	// 2dPhysicsワールドの生成
	m_pPy2dWorld = std::make_unique<physics::d2::box2dWorld>(physics::d2::d2Backend::box2d);
	m_pPy2dWorld->Create();
	// 3dPhysicsワールドの生成
	m_pPy3dWorld.reset(physics::d3::CreateWorld(physics::d3::d3Backend::bullet));
	// システムの生成
	// ECSイベントの登録
	RegisterECSEvents();
	// ECSシステムの登録
	RegisterECSSystems(resourceManager, graphicsEngine);
	// 物理エンジンシステムの無効化
	Rigidbody2DSystem* rigidbody2DSystem = m_pECSManager->GetSystem<Rigidbody2DSystem>();
	rigidbody2DSystem->SetEnabled(false);
	// 衝突関数の登録
	RegisterContactEvents();
	m_EnvironmentData.ambientColor = { 0.01f,0.01f,0.01f,1.0f };
}

void GameCore::Start()
{
	// スクリプト読み込み（場所変更予定）
	theatria::FileSystem::ScriptProject::LoadScriptDLL();
	// クオータニオンに変更
	TransformSystem* transformSystem = m_pECSManager->GetSystem<TransformSystem>();
	transformSystem->m_isQuaternion = true;
	// 物理演算システムの有効化
	Rigidbody2DSystem* rigidbody2DSystem = m_pECSManager->GetSystem<Rigidbody2DSystem>();
	rigidbody2DSystem->SetEnabled(true);
	Rigidbody3DSystem* rigidbody3DSystem = m_pECSManager->GetSystem<Rigidbody3DSystem>();
	rigidbody3DSystem->SetPaused(false);
	// StartSystemの実行
	m_pECSManager->InitializeAllSystems();
}

void GameCore::Update()
{
	if (!m_pSceneManager->GetLoadingSceneName().empty())
	{
		// AwakeSystem
		m_pECSManager->AwakeAllSystems();
		// StartSystemの実行
		m_pECSManager->InitializeAllSystems();
		m_pSceneManager->SetLoadingSceneName(L"");
	}
	// 環境設定の更新
	UpdateEnvironmentSetting();
	// ゲームが実行中でなければreturn
	m_pECSManager->UpdateAllSystems();
	if (isRunning)
	{
		// GameWorld遅延キューの実行
		m_pGameWorld->FlushDeferred();
		// シーン切り替え
		if(!m_pSceneManager->GetLoadingSceneName().empty())
		{
			m_pSceneManager->LoadScene(m_pSceneManager->GetLoadingSceneName(), true);
		}
	}
	
}

void GameCore::Finalize()
{
    theatria::FileSystem::ScriptProject::UnloadScriptDLL();
}

void GameCore::GameRun(const bool& isDebugger, const bool& isRuntime)
{
	if (isRunning)
	{
		return;
	}
	if (!isRuntime)
	{
		// スクリプトプロジェクトの保存処理、ビルド処理
        if (!theatria::FileSystem::ScriptProject::SaveAndBuildSolution(true, isDebugger)) { return; }
	}
	// スクリプト読み込み（場所変更予定）
	theatria::FileSystem::ScriptProject::LoadScriptDLL();
	// クオータニオンに変更
	TransformSystem* transformSystem = m_pECSManager->GetSystem<TransformSystem>();
	transformSystem->m_isQuaternion = true;
	// 物理演算システムの有効化
	Rigidbody2DSystem* rigidbody2DSystem = m_pECSManager->GetSystem<Rigidbody2DSystem>();
	rigidbody2DSystem->SetEnabled(true);
	Rigidbody3DSystem* rigidbody3DSystem = m_pECSManager->GetSystem<Rigidbody3DSystem>();
	rigidbody3DSystem->SetPaused(false);
	// AwakeSystem
	m_pECSManager->AwakeAllSystems();
	// StartSystemの実行
	m_pECSManager->InitializeAllSystems();
	// 実行中フラグを立てる
	isRunning = true;
}

void GameCore::GameStop()
{
	if (!isRunning)
	{
		return;
	}
	// FinalizeSystemの実行
	m_pECSManager->FinalizeAllSystems();
	// 物理演算システムの無効化
	Rigidbody2DSystem* rigidbody2DSystem = m_pECSManager->GetSystem<Rigidbody2DSystem>();
	rigidbody2DSystem->SetEnabled(false);
	Rigidbody3DSystem* rigidbody3DSystem = m_pECSManager->GetSystem<Rigidbody3DSystem>();
	rigidbody3DSystem->SetPaused(true);
	// オイラーに変更
	TransformSystem* transformSystem = m_pECSManager->GetSystem<TransformSystem>();
	transformSystem->m_isQuaternion = false;
	// スクリプトのアンロード（場所変更予定）
	// theatria::FileSystem::ScriptProject::UnloadScriptDLL();
	// デタッチ
	if (theatria::FileSystem::ScriptProject::m_IsAttached)
	{
		theatria::FileSystem::ScriptProject::SendMessageToBuildWatcher(L"STOP_DEBUGGER|" + theatria::FileSystem::m_sProjectName);
		// ここで必ず返事が来るまでブロック
		std::wstring reply = theatria::FileSystem::ScriptProject::WaitForAckFromBuildWatcher(5000); // 5秒待機
		if (!reply.empty())
		{
			Log::Write(LogLevel::Info, L"Received from BuildWatcher: " + reply);
		}
	}
	// 実行中フラグを下ろす
	isRunning = false;
}

void GameCore::UpdateEnvironmentSetting()
{
	// 環境情報バッファ
	ConstantBuffer<BUFFER_DATA_ENVIRONMENT>* envBuffer = m_EngineCommand->GetResourceManager()->GetEnvironmentBuffer();
	envBuffer->UpdateData(m_EnvironmentData);
}

void GameCore::SetScriptSystemEngineCommand()
{
    ScriptSystem* scriptSystem = m_pECSManager->GetSystem<ScriptSystem>();
    scriptSystem->SetConsole(m_EngineCommand->GetEditorManager()->GetConsole());
}

void GameCore::RegisterECSEvents()
{
	m_pComponentEventDispatcher = std::make_shared<ComponentEventDispatcher>(m_EngineCommand);
	m_pComponentEventDispatcher->SetECSManager(m_pECSManager.get());
	m_pECSManager->AddComponentListener(m_pComponentEventDispatcher);
	// イベントの登録
	// TransformComponent
	m_pComponentEventDispatcher->RegisterOnAdd<TransformComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent* c) {
			// Transform統合バッファからmapIDを取得
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Transform)->GetMapID();
			c->mapID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<TransformComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] TransformComponent* c) {
			// Transform統合バッファからmapIDを取得
			TransformComponent& srcTransform = *m_pECSManager->GetComponent<TransformComponent>(src);
			*c = srcTransform;
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Transform)->GetMapID();
			c->mapID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<TransformComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent* c) {
			// Transform統合バッファからmapIDを削除
			m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Transform)->RemoveMapID(c->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<TransformComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent* c) {
			// Transform統合バッファからmapIDを取得
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Transform)->GetMapID();
			c->mapID = mapID;
		});
	// TransformComponentの削除イベントの優先度を下げる
	m_pECSManager->SetDeletionPriority<TransformComponent>(500);
	// CameraComponent
	m_pComponentEventDispatcher->RegisterOnAdd<CameraComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] CameraComponent* c) {
			// Bufferの生成
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateConstantBuffer<BUFFER_DATA_VIEWPROJECTION>();
		});
	m_pComponentEventDispatcher->RegisterOnCopy<CameraComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] CameraComponent* c) {
			CameraComponent& srcCamera = *m_pECSManager->GetComponent<CameraComponent>(src);
			*c = srcCamera;
			// Bufferの生成
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateConstantBuffer<BUFFER_DATA_VIEWPROJECTION>();
		});
	m_pComponentEventDispatcher->RegisterOnRemove<CameraComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] CameraComponent* c) {
			// シーンのMainCameraを削除
			if (m_EngineCommand->GetGameCore()->GetGameWorld()->GetMainCamera()&&
				m_EngineCommand->GetGameCore()->GetGameWorld()->GetMainCamera()->GetHandle().entity == e)
			{
				m_EngineCommand->GetGameCore()->GetGameWorld()->ClearMainCamera();
			}
			// Bufferの削除
			//m_EngineCommand->GetResourceManager()->DeleteConstantBuffer(c->bufferIndex.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<CameraComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] CameraComponent* c) {
			// Bufferの生成
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateConstantBuffer<BUFFER_DATA_VIEWPROJECTION>();
		});
	// MeshFilterComponent
	m_pComponentEventDispatcher->RegisterOnAdd<MeshFilterComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MeshFilterComponent* c) {
			// デフォルトのモデルとしてCubeを設定
			c->modelName = L"Cube";
			// モデルのIDを取得
			c->modelID = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelDataIndex(c->modelName);
			// モデルのUseListに登録
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(e);
			m_EngineCommand->GetResourceManager()->GetModelManager()->RegisterModelUseList(c->modelID.value(), transform->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnCopy<MeshFilterComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] MeshFilterComponent* c) {
			MeshFilterComponent& srcMeshFilter = *m_pECSManager->GetComponent<MeshFilterComponent>(src);
			*c = srcMeshFilter;
			// モデルのIDを取得
			c->modelID = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelDataIndex(c->modelName);
			// モデルのUseListに登録
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(dst);
			
			m_EngineCommand->GetResourceManager()->GetModelManager()->RegisterModelUseList(c->modelID.value(), transform->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRemove<MeshFilterComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MeshFilterComponent* c) {
			// モデルのUseListから削除
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(e);
			m_EngineCommand->GetResourceManager()->GetModelManager()->RemoveModelUseList(c->modelID.value(),transform->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<MeshFilterComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MeshFilterComponent* c) {
			// モデルのIDを取得
			c->modelID = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelDataIndex(c->modelName);
			// モデルのUseListに登録
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(e);
			
			m_EngineCommand->GetResourceManager()->GetModelManager()->RegisterModelUseList(c->modelID.value(), transform->mapID.value());
		});
	// MeshRendererComponent
	m_pComponentEventDispatcher->RegisterOnAdd<MeshRendererComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MeshRendererComponent* c) {
			// 可視化
			c->visible = true;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<MeshRendererComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] MeshRendererComponent* c) {
			MeshRendererComponent& srcMeshRenderer = *m_pECSManager->GetComponent<MeshRendererComponent>(src);
			*c = srcMeshRenderer;
			// 可視化
			c->visible = true;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<MeshRendererComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MeshRendererComponent* c) {
			// 可視化
			c->visible = false;
		});
	m_pComponentEventDispatcher->RegisterOnRestore<MeshRendererComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MeshRendererComponent* c) {
			// 可視化
			c->visible = true;
		});
	// ScriptComponent
	m_pComponentEventDispatcher->RegisterOnAdd<ScriptComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] ScriptComponent* c) {
			//c->objectHandle = m_EngineCommand->GetGameCore()->GetGameWorld()->GetGameObject(e)->GetHandle();
		});
	m_pComponentEventDispatcher->RegisterOnCopy<ScriptComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] ScriptComponent* c) {
			ScriptComponent& srcScript = *m_pECSManager->GetComponent<ScriptComponent>(src);
			*c = srcScript;
			//c->objectHandle = m_EngineCommand->GetGameCore()->GetGameWorld()->GetGameObject(dst)->GetHandle();
		});
	m_pComponentEventDispatcher->RegisterOnRemove<ScriptComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] ScriptComponent* c) {
			// スクリプトのインスタンスを削除
			if (c->instance)
			{
				c->cleanupFunc();
				c->instance = nullptr;
			}
		});
	m_pComponentEventDispatcher->RegisterOnRestore<ScriptComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] ScriptComponent* c) {
			//c->objectHandle = m_EngineCommand->GetGameCore()->GetGameWorld()->GetGameObject(e)->GetHandle();
		});
	// MaterialComponent
	m_pComponentEventDispatcher->RegisterOnAdd<MaterialComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MaterialComponent* c) {
			Color color;
			c->color = color.From255(200, 200, 200);
			c->enableLighting = true;
			c->matUV = float4x4::Identity();
			c->shininess = 50.0f;
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Material)->GetMapID();
			c->mapID = mapID;
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(e);
			transform->materialID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<MaterialComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] MaterialComponent* c) {
			MaterialComponent& srcMaterial = *m_pECSManager->GetComponent<MaterialComponent>(src);
			*c = srcMaterial;
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Material)->GetMapID();
			c->mapID = mapID;
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(dst);
			transform->materialID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<MaterialComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MaterialComponent* c) {
			// Material統合バッファからmapIDを削除
			m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Material)->RemoveMapID(c->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<MaterialComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] MaterialComponent* c) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Material)->GetMapID();
			c->mapID = mapID;
			TransformComponent* transform = m_pECSManager->GetComponent<TransformComponent>(e);
			transform->materialID = mapID;
		});
	// ParticleComponent
	m_pComponentEventDispatcher->RegisterOnAdd<ParticleComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] ParticleComponent* c) {
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateRWStructuredBuffer<BUFFER_DATA_PARTICLE>(c->count);
			c->perFrameBufferIndex = m_EngineCommand->GetResourceManager()->CreateConstantBuffer<BUFFER_DATA_PARTICLE_PERFRAME>();
			c->freeListBufferIndex = m_EngineCommand->GetResourceManager()->CreateRWStructuredBuffer<uint32_t>(c->count, true);
		});
	m_pComponentEventDispatcher->RegisterOnCopy<ParticleComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] ParticleComponent* c) {
			ParticleComponent& srcParticle = *m_pECSManager->GetComponent<ParticleComponent>(src);
			*c = srcParticle;
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateRWStructuredBuffer<BUFFER_DATA_PARTICLE>(c->count);
			c->perFrameBufferIndex = m_EngineCommand->GetResourceManager()->CreateConstantBuffer<BUFFER_DATA_PARTICLE_PERFRAME>();
			c->freeListBufferIndex = m_EngineCommand->GetResourceManager()->CreateRWStructuredBuffer<uint32_t>(c->count, true);
		});
	m_pComponentEventDispatcher->RegisterOnRemove<ParticleComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] ParticleComponent* c) {
			//// Particle統合バッファからmapIDを削除
			//m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Particle)->RemoveMapID(c->bufferIndex.value());
			//// RWStructuredBufferの削除
			//m_EngineCommand->GetResourceManager()->DeleteRWStructuredBuffer(c->bufferIndex.value());
			//m_EngineCommand->GetResourceManager()->DeleteConstantBuffer(c->perFrameBufferIndex.value());
			//m_EngineCommand->GetResourceManager()->DeleteRWStructuredBuffer(c->freeListBufferIndex.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<ParticleComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] ParticleComponent* c) {
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateRWStructuredBuffer<BUFFER_DATA_PARTICLE>(c->count);
			c->perFrameBufferIndex = m_EngineCommand->GetResourceManager()->CreateConstantBuffer<BUFFER_DATA_PARTICLE_PERFRAME>();
			c->freeListBufferIndex = m_EngineCommand->GetResourceManager()->CreateRWStructuredBuffer<uint32_t>(c->count, true);
		});
	// EmitterComponent
	m_pComponentEventDispatcher->RegisterOnAdd<EmitterComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] EmitterComponent* c) {
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateStructuredBuffer<BUFFER_DATA_EMITTER>(1);
		});
	m_pComponentEventDispatcher->RegisterOnCopy<EmitterComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] EmitterComponent* c) {
			EmitterComponent& srcEmitter = *m_pECSManager->GetComponent<EmitterComponent>(src);
			*c = srcEmitter;
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateStructuredBuffer<BUFFER_DATA_EMITTER>(1);
		});
	m_pComponentEventDispatcher->RegisterOnRemove<EmitterComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] EmitterComponent* c) {
			// Emitter統合バッファからmapIDを削除
			//m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Emitter)->RemoveMapID(c->bufferIndex.value());
			// StructuredBufferの削除
			//m_EngineCommand->GetResourceManager()->DeleteStructuredBuffer(c->bufferIndex.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<EmitterComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] EmitterComponent* c) {
			c->bufferIndex = m_EngineCommand->GetResourceManager()->CreateStructuredBuffer<BUFFER_DATA_EMITTER>(1);
		});
	// LineRendererComponent
	m_pComponentEventDispatcher->RegisterOnAdd<LineRendererComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] LineRendererComponent* c, [[maybe_unused]] size_t idx) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Line)->GetMapID();
			c->mapID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<LineRendererComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] LineRendererComponent* c, [[maybe_unused]] size_t idx) {
			LineRendererComponent& srcLine = *m_pECSManager->GetComponent<LineRendererComponent>(src);
			*c = srcLine;
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Line)->GetMapID();
			c->mapID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<LineRendererComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] LineRendererComponent* c, [[maybe_unused]] size_t idx) {
			// Line統合バッファからmapIDを削除
			m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Line)->RemoveMapID(c->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<LineRendererComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] LineRendererComponent* c, [[maybe_unused]] size_t idx) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Line)->GetMapID();
			c->mapID = mapID;
		});
	// Rigidbody2DComponent
	m_pComponentEventDispatcher->RegisterOnAdd<Rigidbody2DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] Rigidbody2DComponent* c) {
			c->selfEntity = e;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<Rigidbody2DComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] Rigidbody2DComponent* c) {
			Rigidbody2DComponent& srcRigidbody = *m_pECSManager->GetComponent<Rigidbody2DComponent>(src);
			*c = srcRigidbody;
			c->selfEntity = dst;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<Rigidbody2DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] Rigidbody2DComponent* c) {
		});
	m_pComponentEventDispatcher->RegisterOnRestore<Rigidbody2DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] Rigidbody2DComponent* c) {
			c->selfEntity = e;
		});
	// BoxCollider2DComponent
	m_pComponentEventDispatcher->RegisterOnAdd<BoxCollider2DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] BoxCollider2DComponent* c) {
		});
	m_pComponentEventDispatcher->RegisterOnCopy<BoxCollider2DComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] BoxCollider2DComponent* c) {
			BoxCollider2DComponent& srcBox = *m_pECSManager->GetComponent<BoxCollider2DComponent>(src);
			*c = srcBox;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<BoxCollider2DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] BoxCollider2DComponent* c) {
		});
	m_pComponentEventDispatcher->RegisterOnRestore<BoxCollider2DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] BoxCollider2DComponent* c) {
		});
	// Rigidbody3DComponent
	m_pComponentEventDispatcher->RegisterOnAdd<Rigidbody3DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] Rigidbody3DComponent* c) {
			c->selfEntity = e;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<Rigidbody3DComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] Rigidbody3DComponent* c) {
			Rigidbody3DComponent& srcRigidbody = *m_pECSManager->GetComponent<Rigidbody3DComponent>(src);
			*c = srcRigidbody;
			c->selfEntity = dst;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<Rigidbody3DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] Rigidbody3DComponent* c) {
		});
	m_pComponentEventDispatcher->RegisterOnRestore<Rigidbody3DComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] Rigidbody3DComponent* c) {
			c->selfEntity = e;
		});
	// EffectComponent
	m_pComponentEventDispatcher->RegisterOnAdd<EffectComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] EffectComponent* c) {
			c->isRun = false;
			c->isLoop = true;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<EffectComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] EffectComponent* c) {
			EffectComponent& srcEffect = *m_pECSManager->GetComponent<EffectComponent>(src);
			*c = srcEffect;
			c->isRun = false;
			c->isLoop = true;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<EffectComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] EffectComponent* c) {
			// Effect統合バッファからmapIDを削除
			//m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::Effect)->RemoveMapID(c->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<EffectComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] EffectComponent* c) {
			c->isRun = false;
			c->isLoop = true;
		});
	// UISpriteComponent
	m_pComponentEventDispatcher->RegisterOnAdd<UISpriteComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] UISpriteComponent* c) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::UISprite)->GetMapID();
			c->mapID = mapID;
			m_EngineCommand->GetResourceManager()->GetUIContainer()->AddUI(mapID);
		});
	m_pComponentEventDispatcher->RegisterOnCopy<UISpriteComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] UISpriteComponent* c) {
			UISpriteComponent& srcUISprite = *m_pECSManager->GetComponent<UISpriteComponent>(src);
			*c = srcUISprite;
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::UISprite)->GetMapID();
			c->mapID = mapID;
			m_EngineCommand->GetResourceManager()->GetUIContainer()->AddUI(mapID);
		});
	m_pComponentEventDispatcher->RegisterOnRemove<UISpriteComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] UISpriteComponent* c) {
			// UISprite統合バッファからmapIDを削除
			m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::UISprite)->RemoveMapID(c->mapID.value());
			m_EngineCommand->GetResourceManager()->GetUIContainer()->RemoveUI(c->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<UISpriteComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] UISpriteComponent* c) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetIntegrationData(IntegrationDataType::UISprite)->GetMapID();
			c->mapID = mapID;
			m_EngineCommand->GetResourceManager()->GetUIContainer()->AddUI(mapID);
		});
	// LightComponent
	m_pComponentEventDispatcher->RegisterOnAdd<LightComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] LightComponent* c) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetLightIndex();
			c->mapID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnCopy<LightComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] LightComponent* c) {
			LightComponent& srcLight = *m_pECSManager->GetComponent<LightComponent>(src);
			*c = srcLight;
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetLightIndex();
			c->mapID = mapID;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<LightComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] LightComponent* c) {
			// Light統合バッファからmapIDを削除
			m_EngineCommand->GetResourceManager()->RecycleLightIndex(c->mapID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<LightComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] LightComponent* c) {
			uint32_t mapID = m_EngineCommand->GetResourceManager()->GetLightIndex();
			c->mapID = mapID;
		});
	// AudioComponent
	m_pComponentEventDispatcher->RegisterOnAdd<AudioComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] AudioComponent* c) {
		});
	m_pComponentEventDispatcher->RegisterOnCopy<AudioComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] AudioComponent* c) {
			AudioComponent& srcAudio = *m_pECSManager->GetComponent<AudioComponent>(src);
			*c = srcAudio;
		});
	m_pComponentEventDispatcher->RegisterOnRemove<AudioComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] AudioComponent* c) {
			for (auto& sound : c->soundData)
			{
				if (sound.isPlaying)
				{
					m_EngineCommand->GetResourceManager()->GetAudioManager()->SoundStop(sound);
				}
			}
		});
	m_pComponentEventDispatcher->RegisterOnRestore<AudioComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] AudioComponent* c) {
		});
	// AnimationComponent
	m_pComponentEventDispatcher->RegisterOnAdd<AnimationComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] AnimationComponent* c) {
			MeshFilterComponent* meshFilter = m_pECSManager->GetComponent<MeshFilterComponent>(e);
			if (!meshFilter) { return; }
			ModelData* model = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelData(meshFilter->modelID.value());
			if (!model) { return; }
			if (model->animations.empty()) { return; }
			c->numAnimation = static_cast<uint32_t>(model->animations.size());
			if (model->isBone)
			{
				c->skeleton = model->skeleton;
				c->skinClusters.clear();
				for (auto& mesh : model->meshes)
				{
					c->skinClusters.push_back(mesh.skinCluster);
				}
				c->modelName = model->name;
				c->boneOffsetID = model->AllocateBoneOffsetIdx();
			}
		});
	m_pComponentEventDispatcher->RegisterOnCopy<AnimationComponent>(
		[&]([[maybe_unused]] Entity src, [[maybe_unused]] Entity dst, [[maybe_unused]] AnimationComponent* c) {
			AnimationComponent& srcAnimation = *m_pECSManager->GetComponent<AnimationComponent>(src);
			*c = srcAnimation;
			MeshFilterComponent* meshFilter = m_pECSManager->GetComponent<MeshFilterComponent>(dst);
			if (!meshFilter) { return; }
			ModelData* model = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelData(meshFilter->modelID.value());
			if (!model) { return; }
			if (model->animations.empty()) { return; }
			c->numAnimation = static_cast<uint32_t>(model->animations.size());
			if (model->isBone)
			{
				c->skeleton = model->skeleton;
				c->skinClusters.clear();
				for (auto& mesh : model->meshes)
				{
					c->skinClusters.push_back(mesh.skinCluster);
				}
				c->modelName = model->name;
				c->boneOffsetID = model->AllocateBoneOffsetIdx();
			}
		});
	m_pComponentEventDispatcher->RegisterOnRemove<AnimationComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] AnimationComponent* c) {
			ModelData* model = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelData(c->modelName);
			model->RemoveBoneOffsetIdx(c->boneOffsetID.value());
		});
	m_pComponentEventDispatcher->RegisterOnRestore<AnimationComponent>(
		[&]([[maybe_unused]] Entity e, [[maybe_unused]] AnimationComponent* c) {
			MeshFilterComponent* meshFilter = m_pECSManager->GetComponent<MeshFilterComponent>(e);
			if (!meshFilter) { return; }
			ModelData* model = m_EngineCommand->GetResourceManager()->GetModelManager()->GetModelData(meshFilter->modelID.value());
			if (!model) { return; }
			if (model->animations.empty()) { return; }
			c->numAnimation = static_cast<uint32_t>(model->animations.size());
			if (model->isBone)
			{
				c->skeleton = model->skeleton;
				c->skinClusters.clear();
				for (auto& mesh : model->meshes)
				{
					c->skinClusters.push_back(mesh.skinCluster);
				}
				c->modelName = model->name;
				c->boneOffsetID = model->AllocateBoneOffsetIdx();
			}
		});
	// Prefabにコンポーネントを登録
	IPrefab::RegisterCopyFunc<TransformComponent>();
	IPrefab::RegisterCopyFunc<CameraComponent>();
	IPrefab::RegisterCopyFunc<MeshFilterComponent>();
	IPrefab::RegisterCopyFunc<MeshRendererComponent>();
	IPrefab::RegisterCopyFunc<ScriptComponent>();
	IPrefab::RegisterCopyFunc<MaterialComponent>();
	IPrefab::RegisterCopyFunc<ParticleComponent>();
	IPrefab::RegisterCopyFunc<EmitterComponent>();
	IPrefab::RegisterCopyFunc<LineRendererComponent>();
	IPrefab::RegisterCopyFunc<Rigidbody2DComponent>();
	IPrefab::RegisterCopyFunc<BoxCollider2DComponent>();
	IPrefab::RegisterCopyFunc<Rigidbody3DComponent>();
	IPrefab::RegisterCopyFunc<EffectComponent>();
	IPrefab::RegisterCopyFunc<UISpriteComponent>();
	IPrefab::RegisterCopyFunc<LightComponent>();
	IPrefab::RegisterCopyFunc<AudioComponent>();
	IPrefab::RegisterCopyFunc<AnimationComponent>();
	IPrefab::RegisterPrefabRestore<TransformComponent>();
	IPrefab::RegisterPrefabRestore<CameraComponent>();
	IPrefab::RegisterPrefabRestore<MeshFilterComponent>();
	IPrefab::RegisterPrefabRestore<MeshRendererComponent>();
	IPrefab::RegisterPrefabRestore<ScriptComponent>();
	IPrefab::RegisterPrefabRestore<MaterialComponent>();
	IPrefab::RegisterPrefabRestore<ParticleComponent>();
	IPrefab::RegisterPrefabRestore<EmitterComponent>();
	IPrefab::RegisterPrefabRestore<LineRendererComponent>();
	IPrefab::RegisterPrefabRestore<Rigidbody2DComponent>();
	IPrefab::RegisterPrefabRestore<BoxCollider2DComponent>();
	IPrefab::RegisterPrefabRestore<Rigidbody3DComponent>();
	IPrefab::RegisterPrefabRestore<EffectComponent>();
	IPrefab::RegisterPrefabRestore<UISpriteComponent>();
	IPrefab::RegisterPrefabRestore<LightComponent>();
	IPrefab::RegisterPrefabRestore<AudioComponent>();
	IPrefab::RegisterPrefabRestore<AnimationComponent>();
}

void GameCore::RegisterECSSystems(ResourceManager* resourceManager, GraphicsEngine* graphicsEngine)
{
	// シングルシステム
	// 初期化システムの登録
	// ScriptComponentSystem
	m_pECSManager->AddSystem<ScriptSystem>();
	ScriptSystem* scriptSystem = m_pECSManager->GetSystem<ScriptSystem>();
	scriptSystem->SetGameWorld(m_pGameWorld.get());
    scriptSystem->SetScriptContainer(resourceManager->GetScriptContainer());
	// TransformComponentSystem
	m_pECSManager->AddSystem<TransformSystem>();
	TransformSystem* transformSystem = m_pECSManager->GetSystem<TransformSystem>();
	transformSystem->SetGameWorld(m_pGameWorld.get());
	transformSystem->SetBuffer(static_cast<StructuredBuffer<BUFFER_DATA_TF>*>(resourceManager->GetIntegrationBuffer(IntegrationDataType::Transform)));
	// AnimationComponentSystem
	m_pECSManager->AddSystem<AnimationSystem>();
	AnimationSystem* animationSystem = m_pECSManager->GetSystem<AnimationSystem>();
	animationSystem->SetResourceManager(resourceManager);
	animationSystem->SetGraphicsEngine(graphicsEngine);
	// CameraComponentSystem
	m_pECSManager->AddSystem<CameraSystem>();
	CameraSystem* cameraSystem = m_pECSManager->GetSystem<CameraSystem>();
	cameraSystem->SetResourceManager(resourceManager);
	cameraSystem->SetBuffer(static_cast<StructuredBuffer<BUFFER_DATA_TF>*>(resourceManager->GetIntegrationBuffer(IntegrationDataType::Transform)));
	// MaterialComponentSystem
	m_pECSManager->AddSystem<MaterialSystem>();
	MaterialSystem* materialSystem = m_pECSManager->GetSystem<MaterialSystem>();
	materialSystem->SetBuffer(static_cast<StructuredBuffer<BUFFER_DATA_MATERIAL>*>(resourceManager->GetIntegrationBuffer(IntegrationDataType::Material)));
	materialSystem->SetResourceManager(resourceManager);
	// LightComponentSystem
	m_pECSManager->AddSystem<LightSystem>();
	LightSystem* lightSystem = m_pECSManager->GetSystem<LightSystem>();
	lightSystem->SetResourceManager(resourceManager);
	// UISpriteComponentSystem
	m_pECSManager->AddSystem<UISpriteSystem>();
	UISpriteSystem* uiSpriteSystem = m_pECSManager->GetSystem<UISpriteSystem>();
	uiSpriteSystem->SetResourceManager(resourceManager);
	uiSpriteSystem->SetBuffer(static_cast<StructuredBuffer<BUFFER_DATA_UISPRITE>*>(resourceManager->GetIntegrationBuffer(IntegrationDataType::UISprite)));
	// ParticleComponentSystem
	m_pECSManager->AddSystem<ParticleEmitterSystem>();
	ParticleEmitterSystem* particleEmitterSystem = m_pECSManager->GetSystem<ParticleEmitterSystem>();
	particleEmitterSystem->SetResourceManager(resourceManager);
	particleEmitterSystem->SetGraphicsEngine(graphicsEngine);
	// EffectEditorComponentSystem
	m_pECSManager->AddSystem<EffectEditorSystem>();
	EffectEditorSystem* effectEditorSystem = m_pECSManager->GetSystem<EffectEditorSystem>();
	effectEditorSystem->SetGraphicsEngine(graphicsEngine);
	effectEditorSystem->SetResourceManager(resourceManager);
	// rigidbody2DComponentSystem
	m_pECSManager->AddSystem<Rigidbody2DSystem>();
	Rigidbody2DSystem* rigidbody2DSystem = m_pECSManager->GetSystem<Rigidbody2DSystem>();
	rigidbody2DSystem->SetPhysicsWorld(m_pPy2dWorld.get());
	// Collider2DComponentSystem
	m_pECSManager->AddSystem<Collider2DSystem>();
	// rigidbody3DComponentSystem
	m_pECSManager->AddSystem<Rigidbody3DSystem>();
	Rigidbody3DSystem* rigidbody3DSystem = m_pECSManager->GetSystem<Rigidbody3DSystem>();
	rigidbody3DSystem->SetPhysicsWorld(m_pPy3dWorld.get());
	// CollisionSystem
	m_pECSManager->AddSystem<CollisionSystem>();
	CollisionSystem* collisionSystem = m_pECSManager->GetSystem<CollisionSystem>();
	collisionSystem->SetGameWorld(m_pGameWorld.get());
	// LineRendererComponentSystem
	m_pECSManager->AddSystem<LineRendererSystem>();
	LineRendererSystem* lineRendererSystem = m_pECSManager->GetSystem<LineRendererSystem>();
	lineRendererSystem->SetResourceManager(resourceManager);
	lineRendererSystem->SetBuffer(static_cast<VertexBuffer<BUFFER_DATA_LINE>*>(resourceManager->GetLineIntegrationBuffer()));

}

void GameCore::RegisterContactEvents()
{
	m_pPy2dWorld->SetBeginContactCallback([this](void* a, void* b)
	{
			// 接触開始処理
			if (a && b)
			{
				Entity entityA = *static_cast<Entity*>(a);
				Entity entityB = *static_cast<Entity*>(b);

				// CollisionEnter
				ScriptComponent* scriptA = m_pECSManager->GetComponent<ScriptComponent>(entityA);
				ScriptComponent* scriptB = m_pECSManager->GetComponent<ScriptComponent>(entityB);
				Rigidbody2DComponent* rigidbodyA = m_pECSManager->GetComponent<Rigidbody2DComponent>(entityA);
				Rigidbody2DComponent* rigidbodyB = m_pECSManager->GetComponent<Rigidbody2DComponent>(entityB);
				if (rigidbodyA && rigidbodyA->isActive && rigidbodyB && rigidbodyB->isActive)
				{
					if (scriptA && scriptA->instance)
					{
						GameObject* gameObjectB = m_pGameWorld->GetGameObject(entityB);
						scriptA->onCollisionEnterFunc(*gameObjectB);
					}
					if(scriptB && scriptB->instance)
					{
						GameObject* gameObjectA = m_pGameWorld->GetGameObject(entityA);
						scriptB->onCollisionEnterFunc(*gameObjectA);
					}
				}
			}
		});
	m_pPy2dWorld->SetEndContactCallback([this](void* a, void* b)
	{
			// 接触終了処理
			if (a && b)
			{
				Entity entityA = *static_cast<Entity*>(a);
				Entity entityB = *static_cast<Entity*>(b);

				// CollisionEnter
				ScriptComponent* scriptA = m_pECSManager->GetComponent<ScriptComponent>(entityA);
				ScriptComponent* scriptB = m_pECSManager->GetComponent<ScriptComponent>(entityB);
				Rigidbody2DComponent* rigidbodyA = m_pECSManager->GetComponent<Rigidbody2DComponent>(entityA);
				Rigidbody2DComponent* rigidbodyB = m_pECSManager->GetComponent<Rigidbody2DComponent>(entityB);
				if (rigidbodyA && rigidbodyA->isActive && rigidbodyB && rigidbodyB->isActive)
				{
					if (scriptA && scriptA->instance)
					{
						GameObject* gameObjectB = m_pGameWorld->GetGameObject(entityB);
						scriptA->onCollisionExitFunc(*gameObjectB);
					}
					if (scriptB && scriptB->instance)
					{
						GameObject* gameObjectA = m_pGameWorld->GetGameObject(entityA);
						scriptB->onCollisionExitFunc(*gameObjectA);
					}
				}
			}
		});
	m_pPy2dWorld->SetStayContactCallback([this](void* a, void* b)
	{
			// 接触中処理
			if (a && b)
			{
				Entity entityA = *static_cast<Entity*>(a);
				Entity entityB = *static_cast<Entity*>(b);

				// CollisionEnter
				ScriptComponent* scriptA = m_pECSManager->GetComponent<ScriptComponent>(entityA);
				ScriptComponent* scriptB = m_pECSManager->GetComponent<ScriptComponent>(entityB);
				Rigidbody2DComponent* rigidbodyA = m_pECSManager->GetComponent<Rigidbody2DComponent>(entityA);
				Rigidbody2DComponent* rigidbodyB = m_pECSManager->GetComponent<Rigidbody2DComponent>(entityB);
				if (rigidbodyA && rigidbodyA->isActive && rigidbodyB && rigidbodyB->isActive)
				{
					if (scriptA && scriptA->instance)
					{
						GameObject* gameObjectB = m_pGameWorld->GetGameObject(entityB);
						scriptA->onCollisionStayFunc(*gameObjectB);
					}
					if (scriptB && scriptB->instance)
					{
						GameObject* gameObjectA = m_pGameWorld->GetGameObject(entityA);
						scriptB->onCollisionStayFunc(*gameObjectA);
					}
				}
			}
		});

	m_pPy3dWorld->SetBeginContactCallback([this](int a, int b)
		{
			// 接触開始処理
			if (a && b)
			{
				Entity entityA = static_cast<Entity>(a);
				Entity entityB = static_cast<Entity>(b);

				// CollisionEnter
				ScriptComponent* scriptA = m_pECSManager->GetComponent<ScriptComponent>(entityA);
				ScriptComponent* scriptB = m_pECSManager->GetComponent<ScriptComponent>(entityB);
				Rigidbody3DComponent* rigidbodyA = m_pECSManager->GetComponent<Rigidbody3DComponent>(entityA);
				Rigidbody3DComponent* rigidbodyB = m_pECSManager->GetComponent<Rigidbody3DComponent>(entityB);
				if (rigidbodyA && rigidbodyA->IsActive() && rigidbodyB && rigidbodyB->IsActive())
				{
					if (scriptA && scriptA->instance)
					{
						GameObject* gameObjectB = m_pGameWorld->GetGameObject(entityB);
						scriptA->onCollisionEnterFunc(*gameObjectB);
					}
					if (scriptB && scriptB->instance)
					{
						GameObject* gameObjectA = m_pGameWorld->GetGameObject(entityA);
						scriptB->onCollisionEnterFunc(*gameObjectA);
					}
				}
			}
		});
	m_pPy3dWorld->SetEndContactCallback([this](int a, int b)
		{
			// 接触終了処理
			if (a && b)
			{
				Entity entityA = static_cast<Entity>(a);
				Entity entityB = static_cast<Entity>(b);

				// CollisionEnter
				ScriptComponent* scriptA = m_pECSManager->GetComponent<ScriptComponent>(entityA);
				ScriptComponent* scriptB = m_pECSManager->GetComponent<ScriptComponent>(entityB);
				Rigidbody3DComponent* rigidbodyA = m_pECSManager->GetComponent<Rigidbody3DComponent>(entityA);
				Rigidbody3DComponent* rigidbodyB = m_pECSManager->GetComponent<Rigidbody3DComponent>(entityB);
				if (rigidbodyA && rigidbodyA->IsActive() && rigidbodyB && rigidbodyB->IsActive())
				{
					if (scriptA && scriptA->instance)
					{
						GameObject* gameObjectB = m_pGameWorld->GetGameObject(entityB);
						scriptA->onCollisionExitFunc(*gameObjectB);
					}
					if (scriptB && scriptB->instance)
					{
						GameObject* gameObjectA = m_pGameWorld->GetGameObject(entityA);
						scriptB->onCollisionExitFunc(*gameObjectA);
					}
				}
			}
		});
	m_pPy3dWorld->SetStayContactCallback([this](int a, int b)
		{
			// 接触中処理
			if (a && b)
			{
				Entity entityA = static_cast<Entity>(a);
				Entity entityB = static_cast<Entity>(b);	

				// CollisionEnter
				ScriptComponent* scriptA = m_pECSManager->GetComponent<ScriptComponent>(entityA);
				ScriptComponent* scriptB = m_pECSManager->GetComponent<ScriptComponent>(entityB);
				Rigidbody3DComponent* rigidbodyA = m_pECSManager->GetComponent<Rigidbody3DComponent>(entityA);
				Rigidbody3DComponent* rigidbodyB = m_pECSManager->GetComponent<Rigidbody3DComponent>(entityB);
				if (rigidbodyA && rigidbodyA->IsActive() && rigidbodyB && rigidbodyB->IsActive())
				{
					if (scriptA && scriptA->instance)
					{
						GameObject* gameObjectB = m_pGameWorld->GetGameObject(entityB);
						scriptA->onCollisionStayFunc(*gameObjectB);
					}
					if (scriptB && scriptB->instance)
					{
						GameObject* gameObjectA = m_pGameWorld->GetGameObject(entityA);
						scriptB->onCollisionStayFunc(*gameObjectA);
					}
				}
			}
		});
}
