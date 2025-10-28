#include "pch.h"
#include "EngineCommands.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "GameCore/GameCore.h"
#include "Editor/EditorManager/EditorManager.h"
#include "Core/Utility/GenerateUnique.h"
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;
#include <random>

bool Add3DObjectCommand::Execute(EngineCommand* edit)
{
	// デフォルトの名前
	std::wstring name = L"NewMeshObject";
	// Worldに追加
	m_Handle = edit->GetGameCore()->GetGameWorld()->CreateGameObject(name, ObjectType::MeshObject);
	// SelectedObjectを設定
	edit->GetEditorManager()->SetSelectedGameObject(edit->GetGameCore()->GetGameWorld()->GetGameObject(m_Handle));
	return true;
}

bool Add3DObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddCameraObjectCommand::Execute(EngineCommand* edit)
{
	// デフォルトの名前
	std::wstring name = L"NewCameraObject";
	// Worldに追加
	m_Handle = edit->GetGameCore()->GetGameWorld()->CreateGameObject(name, ObjectType::Camera);
	// SelectedObjectを設定
	edit->GetEditorManager()->SetSelectedGameObject(edit->GetGameCore()->GetGameWorld()->GetGameObject(m_Handle));
	return true;
}

bool AddCameraObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddMeshFilterComponent::Execute(EngineCommand* edit)
{
	// MeshFilterComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<MeshFilterComponent>(m_Entity);
	return true;
}

bool AddMeshFilterComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddMeshRendererComponent::Execute(EngineCommand* edit)
{
	// MeshRendererComponentを追加
	MeshRendererComponent* render = edit->m_GameCore->GetECSManager()->AddComponent<MeshRendererComponent>(m_Entity);
	render->visible = true;
	return true;
}

bool AddMeshRendererComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool SetMainCamera::Execute(EngineCommand* edit)
{
	// 現在のMainCameraを取得
	m_PreCameraHandle = edit->m_GameCore->GetGameWorld()->GetMainCamera()->GetHandle();
	// MainCameraを取得
	GameObject* camera = edit->m_GameCore->GetGameWorld()->GetGameObject(m_SetCameraHandle);
	// MainCameraを設定
	edit->m_GameCore->GetGameWorld()->SetMainCamera(camera);

	return true;
}

bool SetMainCamera::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddScriptComponent::Execute(EngineCommand* edit)
{
	// ScriptComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<ScriptComponent>(m_Entity);
	return true;
}

bool AddScriptComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddLineRendererComponent::Execute(EngineCommand* edit)
{
	// LineRendererComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<LineRendererComponent>(m_Entity);
	return false;
}

bool AddLineRendererComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddRigidbody2DComponent::Execute(EngineCommand* edit)
{
	// Rigidbody2DComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<Rigidbody2DComponent>(m_Entity);
	return true;
}

bool AddRigidbody2DComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddBoxCollider2DComponent::Execute(EngineCommand* edit)
{
	// BoxCollider2DComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<BoxCollider2DComponent>(m_Entity);
	return true;
}

bool AddBoxCollider2DComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool DeleteObjectCommand::Execute(EngineCommand* edit)
{
	ECSManager* ecs = edit->m_GameCore->GetECSManager();
	GameObject* object = edit->m_GameCore->GetGameWorld()->GetGameObject(m_Handle);
	// Prefab作成
	m_Prefab = std::make_unique<CPrefab>(CPrefab::FromEntity(*ecs, object->GetHandle().entity,object->GetName(),object->GetType(),object->GetTag()));
	// GameWorldから削除
	edit->m_GameCore->GetGameWorld()->RemoveGameObject(m_Handle);
	return true;
}

bool DeleteObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool RenameObjectCommand::Execute(EngineCommand* edit)
{
	// GameObjectを取得
	GameObject* object = edit->m_GameCore->GetGameWorld()->GetGameObject(m_Handle);
	// 変更前の名前を保存
	m_PreName = object->GetName();
	// 名前変更
	edit->m_GameCore->GetGameWorld()->RenameGameObject(m_Handle, m_NewName);
	return true;
}

bool RenameObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddMaterialComponent::Execute(EngineCommand* edit)
{
	// MaterialComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<MaterialComponent>(m_Entity);
	return true;
}

bool AddMaterialComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddParticleSystemObjectCommand::Execute(EngineCommand* edit)
{
	// デフォルトの名前
	std::wstring name = L"NewParticleSystem";
	// Worldに追加
	m_Handle = edit->GetGameCore()->GetGameWorld()->CreateGameObject(name, ObjectType::ParticleSystem);
	// SelectedObjectを設定
	edit->GetEditorManager()->SetSelectedGameObject(edit->GetGameCore()->GetGameWorld()->GetGameObject(m_Handle));
	EmitterComponent* emitter = edit->m_GameCore->GetECSManager()->GetComponent<EmitterComponent>(m_Handle.entity);
	emitter->scale.value.x.median = 1.0f;
	emitter->scale.value.y.median = 1.0f;
	emitter->scale.value.z.median = 1.0f;
	emitter->lifeTime.median = 20.0f;
	return true;
}

bool AddParticleSystemObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddEmitterComponent::Execute(EngineCommand* edit)
{
	// EmitterComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<EmitterComponent>(m_Entity);
	return true;
}

bool AddEmitterComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddParticleComponent::Execute(EngineCommand* edit)
{
	// ParticleComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<ParticleComponent>(m_Entity);
	return true;
}

bool AddParticleComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddEffectObjectCommand::Execute(EngineCommand* edit)
{
	// デフォルトの名前
	std::wstring name = L"EditorEffect";
	// Worldに追加
	m_Handle = edit->GetGameCore()->GetGameWorld()->CreateGameObject(name, ObjectType::Effect);
	// SelectedObjectを設定
	edit->GetEditorManager()->SetSelectedGameObject(edit->GetGameCore()->GetGameWorld()->GetGameObject(m_Handle));
	return true;
}

bool AddEffectObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddUIObjectCommand::Execute(EngineCommand* edit)
{
	// デフォルトの名前
	std::wstring name = L"NewUI";
	// Worldに追加
	m_Handle = edit->GetGameCore()->GetGameWorld()->CreateGameObject(name, ObjectType::UI);
	// SelectedObjectを設定
	edit->GetEditorManager()->SetSelectedGameObject(edit->GetGameCore()->GetGameWorld()->GetGameObject(m_Handle));
	return true;
}

bool AddUIObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool SetGravityCommand::Execute(EngineCommand* edit)
{
	physics::d2::Id2World* world = edit->m_GameCore->GetPhysicsWorld2D();
	physics::d3::Id3World* world3D = edit->m_GameCore->GetPhysicsWorld3D();
	if (!world) { return false; }
	if (!world3D) { return false; }
	// 重力を取得
	float3 gravity = world3D->GetGravity();
	m_PreGravity = float3(gravity.x, gravity.y,gravity.z);
	// 重力を設定
	world->SetGravity(float2(m_Gravity.x, m_Gravity.y));
	world3D->SetGravity(m_Gravity);
	return true;
}

bool SetGravityCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddLightObjectCommand::Execute(EngineCommand* edit)
{
	// デフォルトの名前
	std::wstring name = L"NewLight";
	// Worldに追加
	m_Handle = edit->GetGameCore()->GetGameWorld()->CreateGameObject(name, ObjectType::UI);
	// SelectedObjectを設定
	edit->GetEditorManager()->SetSelectedGameObject(edit->GetGameCore()->GetGameWorld()->GetGameObject(m_Handle));
	return true;
}

bool AddLightObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddAudioComponent::Execute(EngineCommand* edit)
{
	// AudioComponentを追加
	AudioComponent* audio = edit->m_GameCore->GetECSManager()->AddComponent<AudioComponent>(m_Entity);
	if (!audio) { return false; }
	return true;
}

bool AddAudioComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddAnimationComponent::Execute(EngineCommand* edit)
{
	// AnimationComponentを追加
	MeshFilterComponent* meshFilter = edit->m_GameCore->GetECSManager()->GetComponent<MeshFilterComponent>(m_Entity);
	if (!meshFilter) { return false; }
	ModelData* model = edit->m_ResourceManager->GetModelManager()->GetModelData(meshFilter->modelID.value());
	if (!model) { return false; }
	if (model->animations.empty()) { return false; }
	AnimationComponent* animation = edit->m_GameCore->GetECSManager()->AddComponent<AnimationComponent>(m_Entity);
	if (!animation) { return false; }
	/*if (model->isBone)
	{
		animation->skeleton = model->skeleton;
		animation->skinCluster = model->skinCluster;
		animation->modelName = model->name;
		animation->boneOffsetID = model->AllocateBoneOffsetIdx();
	}*/
	
	return true;
}

bool AddAnimationComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool CreateEffectCommand::Execute(EngineCommand* edit)
{
	// 作成中のEffectObjectがあるなら失敗
	if (edit->GetEditorManager()->GetEffectEntity().has_value()) { return false; }
	// 各IDの取得
	// Entity
	Entity entity = edit->m_GameCore->GetECSManager()->GenerateEntity();
	m_Entity = entity;
	// デフォルトの名前
	std::wstring name = L"EditorEffect";
	// EffectComponentを追加
	EffectComponent* effect = edit->m_GameCore->GetECSManager()->AddComponent<EffectComponent>(entity);
	if (!effect) { return false; }
	edit->GetEditorManager()->SetEffectEntity(entity);
	// Rootを取得
	uint32_t rootID = edit->m_ResourceManager->GetIntegrationData(IntegrationDataType::EffectRootInt)->GetMapID();
	effect->root = std::make_pair(rootID, EffectRootData());
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(1, 1000);
	effect->root.second.globalSeed = static_cast<uint32_t>(dist(gen));
	// UseListに登録
	edit->m_ResourceManager->AddEffectRootUseList(rootID);
	// Nodeを一つ追加
	uint32_t nodeID = edit->m_ResourceManager->GetIntegrationData(IntegrationDataType::EffectNodeInt)->GetMapID();
	effect->root.second.nodeCount++;
	EffectNodeData nodedata;
	nodedata.nodeID = nodeID;
	// Rootを親に設定
	nodedata.isRootParent = 1;
	// 初期はSprite
	uint32_t spriteID = edit->m_ResourceManager->GetIntegrationData(IntegrationDataType::EffectSpriteInt)->GetMapID();
	nodedata.draw.meshDataIndex = spriteID;
	nodedata.draw.meshType = static_cast<uint32_t>(EFFECT_MESH_TYPE::SPRITE);
	EffectSprite sprite;
	nodedata.drawMesh = sprite;
	nodedata.name = "NewNode";
	// 各パラメータの初期値
	nodedata.common.lifeTime.median = 60.0f;
	nodedata.common.emitCountMax = 20;
	nodedata.scale.value = float3(1.0f, 1.0f, 1.0f);
	nodedata.scale.pva.value.x.median = 1.0f;
	nodedata.scale.pva.value.y.median = 1.0f;
	nodedata.scale.pva.value.z.median = 1.0f;
	effect->root.second.nodes.push_back(std::move(nodedata));
	return true;
}

bool CreateEffectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddEffectNodeCommand::Execute(EngineCommand* edit)
{
	// 作成中のEffectObjectがないなら失敗
	if (!edit->GetEditorManager()->GetEffectEntity().has_value()) { return false; }
	// EffectComponentを取得
	EffectComponent* effect = edit->m_GameCore->GetECSManager()->GetComponent<EffectComponent>(edit->GetEditorManager()->GetEffectEntity().value());
	if (!effect) { return false; }
	// 新しいNodeのIDを取得
	uint32_t nodeID = edit->m_ResourceManager->GetIntegrationData(IntegrationDataType::EffectNodeInt)->GetMapID();
	// EffectRootのNodeIDに追加
	effect->root.second.nodeCount++;
	EffectNodeData nodedata;
	nodedata.nodeID = nodeID;
	// 選択中のNodeを親に設定
	if (edit->GetEditorManager()->GetEffectNodeID().has_value())
	{
		nodedata.isRootParent = 0;
		nodedata.parentIndex = effect->root.second.nodes[edit->GetEditorManager()->GetEffectNodeID().value()].nodeID;
	}
	else
	{
		// 親がないならRootを親に設定
		nodedata.isRootParent = 1;
	}
	// 初期はSprite
	uint32_t spriteID = edit->m_ResourceManager->GetIntegrationData(IntegrationDataType::EffectSpriteInt)->GetMapID();
	nodedata.draw.meshDataIndex = spriteID;
	nodedata.draw.meshType = static_cast<uint32_t>(EFFECT_MESH_TYPE::SPRITE);
	EffectSprite sprite;
	nodedata.drawMesh = sprite;
	nodedata.name = "NewNode";
	nodedata.scale.value = float3(1.0f, 1.0f, 1.0f);
	nodedata.scale.pva.value.x.median = 1.0f;
	nodedata.scale.pva.value.y.median = 1.0f;
	nodedata.scale.pva.value.z.median = 1.0f;
	// Nodeを追加
	effect->root.second.nodes.push_back(std::move(nodedata));
	// 追加したNodeを選択中に設定
	edit->GetEditorManager()->SetEffectNodeIndex(nodeID);
	return true;
}

bool AddEffectNodeCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool CloneObjectCommand::Execute(EngineCommand* edit)
{
	m_Dst = edit->GetGameCore()->GetGameWorld()->AddGameObjectClone(m_Src);
	return true;
}

bool CloneObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool CopyObjectCommand::Execute(EngineCommand* edit)
{
	m_Dst = edit->GetGameCore()->GetGameWorld()->CreateGameObjectCopy(m_Src);
	return true;
}

bool CopyObjectCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool ChangeEditorSceneCommand::Execute(EngineCommand* edit)
{
	// 編集中のシーンを取得
	std::wstring editSceneName = edit->GetEditorManager()->GetEditingSceneName();
	// 変更前のシーンの名前を保存
	m_PreSceneName = editSceneName;
	// EditorManagerの編集中のシーンを更新
	edit->GetEditorManager()->ChangeEditingScene(m_SceneName);
	return true;
}

bool ChangeEditorSceneCommand::Undo(EngineCommand* edit)
{
	edit;
	return false;
}

bool AddRigidbody3DComponent::Execute(EngineCommand* edit)
{
	// Rigidbody3DComponentを追加
	edit->m_GameCore->GetECSManager()->AddComponent<Rigidbody3DComponent>(m_Entity);
	return true;
}

bool AddRigidbody3DComponent::Undo(EngineCommand* edit)
{
	edit;
	return false;
}
