#pragma once
#include "GameCore/ECS/ECSManager.h"
#include "SDK/DirectX/DirectX12/GpuBuffer/GpuBuffer.h"
#include "Core/Utility/CompBufferData.h"
#include "Core/Utility/Components.h"
#include <2D/d2_common.h>
#include <3D/d3_common.h>

// 前方宣言
class ResourceManager;
class GraphicsEngine;
class GameWorld;
class EngineCommand;
class ScriptContainer;
class Console;
struct ModelData;

// TransformComponentSystem
class TransformSystem : public ECSManager::System<TransformComponent>
{
	friend class GameCore;
public:
	TransformSystem():
		ECSManager::System<TransformComponent>(
			[&](Entity e, TransformComponent& transform)
			{
				UpdateComponent(e, transform);
			},
			[&](Entity e, TransformComponent& transform)
			{
				InitializeComponent(e, transform);
			},
			[&](Entity e, TransformComponent& transform)
			{
				FinalizeComponent(e, transform);
			})
	{
	}
private:
	void InitializeComponent([[maybe_unused]] Entity e,[[maybe_unused]] TransformComponent& transform);
	// 優先度順に更新する
	void priorityUpdate();
	void UpdateComponent([[maybe_unused]] Entity e, TransformComponent& transform);
	// 転送
	void TransferMatrix(TransformComponent& transform);
	void FinalizeComponent([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent& transform);

	void SetBuffer(StructuredBuffer<BUFFER_DATA_TF>* buffer)
	{
		m_pIntegrationBuffer = buffer;
	}

	void SetGameWorld(GameWorld* gameWorld)
	{
		m_pGameWorld = gameWorld;
	}

	StructuredBuffer<BUFFER_DATA_TF>* m_pIntegrationBuffer = nullptr;
	GameWorld* m_pGameWorld = nullptr; // GameWorldへのポインタ
	bool m_isQuaternion = false; // クォータニオンを使用するかどうか
};

class CameraSystem : public ECSManager::System<TransformComponent, CameraComponent>
{
	friend class GameCore;
	public:
		CameraSystem():
			ECSManager::System<TransformComponent, CameraComponent>(
				[&](Entity e, TransformComponent& transform, CameraComponent& camera)
				{
					UpdateComponent(e, transform, camera);
				},
				[&](Entity e, TransformComponent& transform, CameraComponent& camera)
				{
					InitializeComponent(e, transform, camera);
				},
				[&](Entity e, TransformComponent& transform, CameraComponent& camera)
				{
					FinalizeComponent(e, transform, camera);
				})
		{
		}
	~CameraSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, TransformComponent& transform, CameraComponent& camera);
	void UpdateComponent([[maybe_unused]] Entity e, TransformComponent& transform, CameraComponent& camera);
	void TransferMatrix(TransformComponent& transform, CameraComponent& camera);
	void FinalizeComponent([[maybe_unused]] Entity e, TransformComponent& transform, CameraComponent& camera);

	void SetBuffer(StructuredBuffer<BUFFER_DATA_TF>* buffer)
	{
		m_pIntegrationBuffer = buffer;
	}
	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	bool m_isQuaternion = false; // クォータニオンを使用するかどうか
	ResourceManager* m_pResourceManager = nullptr;
	StructuredBuffer<BUFFER_DATA_TF>* m_pIntegrationBuffer = nullptr;
};

class ScriptSystem : public ECSManager::System<ScriptComponent>
{
	friend class GameCore;
public:
	ScriptSystem():
		ECSManager::System<ScriptComponent>(
			[&](Entity e, ScriptComponent& script)
			{
				UpdateComponent(e, script);
			},
			[&](Entity e, ScriptComponent& script)
			{
				InitializeComponent(e, script);
			},
			[&](Entity e, ScriptComponent& script)
			{
				FinalizeComponent(e, script);
			},
			[&](Entity e, ScriptComponent& script)
			{
				AwakeComponent(e, script);
			}
		)
	{
	}
	~ScriptSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, [[maybe_unused]] ScriptComponent& script);
	void UpdateComponent([[maybe_unused]] Entity e, ScriptComponent& script);
	void FinalizeComponent([[maybe_unused]] Entity e, ScriptComponent& script);
	void AwakeComponent([[maybe_unused]] Entity e, ScriptComponent& script);

	void SetGameWorld(GameWorld* gameWorld)
	{
		m_pGameWorld = gameWorld;
	}
    void SetScriptContainer(ScriptContainer* container)
    {
        m_pScriptContainer = container;
    }
    // Editorのみ
    void SetConsole(Console* console)
    {
        m_pConsole = console;
    }

	GameWorld* m_pGameWorld = nullptr;
    ScriptContainer* m_pScriptContainer = nullptr;
    Console* m_pConsole = nullptr;
};

class Rigidbody2DSystem : public ECSManager::System<TransformComponent, Rigidbody2DComponent>
{
	friend class GameCore;
public:
	Rigidbody2DSystem():
		ECSManager::System<TransformComponent, Rigidbody2DComponent>(
			[this](Entity e, TransformComponent& transform, Rigidbody2DComponent& rb)
			{
				UpdateComponent(e, transform, rb);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody2DComponent& rb)
			{
				InitializeComponent(e, transform, rb);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody2DComponent& rb)
			{
				FinalizeComponent(e, transform, rb);
			})
	{
	}
	~Rigidbody2DSystem() = default;
	void Update() override
	{
		// Step は一回だけ呼ぶ（エンティティループより前）
		StepSimulation();

		// いつもの処理（b2Body -> Transform同期）
		ECSManager::System<TransformComponent, Rigidbody2DComponent>::Update();
	}
private:
	void InitializeComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody2DComponent& rb);
	void StepSimulation();
	void UpdateComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody2DComponent& rb);
	void Reset(Rigidbody2DComponent& rb);
	void FinalizeComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody2DComponent& rb);

	void SetPhysicsWorld(physics::d2::Id2World* world)
	{
		m_World = world;
	}

	physics::d2::Id2World* m_World = nullptr;
};

class CollisionSystem : public ECSManager::System<ScriptComponent, Rigidbody2DComponent>
{
	friend class GameCore;
public:
	CollisionSystem() :
		ECSManager::System<ScriptComponent, Rigidbody2DComponent>(
			[&]([[maybe_unused]] Entity e, [[maybe_unused]] ScriptComponent& script, [[maybe_unused]] Rigidbody2DComponent& rb)
			{
			},
			[&]([[maybe_unused]] Entity e, ScriptComponent& script, Rigidbody2DComponent& rb)
			{
				CollisionStay(script, rb);
			},
			[&]([[maybe_unused]] Entity e, [[maybe_unused]] ScriptComponent& script, [[maybe_unused]] Rigidbody2DComponent& rb)
			{
			})
	{
	}
	~CollisionSystem() = default;
private:
	void CollisionStay(ScriptComponent& script, Rigidbody2DComponent& rb);

	void SetGameWorld(GameWorld* gameWorld)
	{
		m_pGameWorld = gameWorld;
	}
	
	GameWorld* m_pGameWorld = nullptr;
};

class Collider2DSystem : public ECSManager::System<TransformComponent, Rigidbody2DComponent, BoxCollider2DComponent>
{
public:
	Collider2DSystem():
		ECSManager::System<TransformComponent, Rigidbody2DComponent, BoxCollider2DComponent>(
			[this](Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box)
			{
				UpdateComponent(e, transform, rb, box);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box)
			{
				e;
				InitializeComponent(e, transform, rb, box);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box)
			{
				e;
				FinalizeComponent(e, transform, rb, box);
			})
	{
	}
	~Collider2DSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box);
	void UpdateComponent([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box);
	void FinalizeComponent([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent& transform, [[maybe_unused]] Rigidbody2DComponent& rb, [[maybe_unused]] BoxCollider2DComponent& box);
};

class Rigidbody3DSystem : public ECSManager::System<TransformComponent, Rigidbody3DComponent>
{
	friend class GameCore;
public:
	Rigidbody3DSystem():
		ECSManager::System<TransformComponent, Rigidbody3DComponent>(
			[this](Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
			{
				UpdateComponent(e, transform, rb);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
			{
				InitializeComponent(e, transform, rb);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
			{
				FinalizeComponent(e, transform, rb);
			},
			[this](Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
			{
				AwakeComponent(e, transform, rb);
			}
		)
	{
	}
	~Rigidbody3DSystem() = default;
	void Update() override
	{
		// Step は一回だけ呼ぶ（エンティティループより前）
		StepSimulation();

		// いつもの処理
		ECSManager::System<TransformComponent, Rigidbody3DComponent>::Update();
	}
	void SetPaused(bool paused)
	{
		m_Paused = paused;
	}
private:
	void InitializeComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody3DComponent& rb);
	void UpdateComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody3DComponent& rb);
	void Reset(Rigidbody3DComponent& rb);
	void FinalizeComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody3DComponent& rb);
	void StepSimulation();
	void AwakeComponent([[maybe_unused]] Entity e, TransformComponent& transform, Rigidbody3DComponent& rb);
	void SetPhysicsWorld(physics::d3::Id3World* world)
	{
		m_World = world;
	}
	physics::d3::Id3World* m_World = nullptr;
	bool m_Paused = true; // 一時停止フラグ
};

class MaterialSystem : public ECSManager::System<MaterialComponent>
{
	friend class GameCore;
public:
	MaterialSystem() : 
		ECSManager::System<MaterialComponent>(
			[&](Entity e, MaterialComponent& material)
			{
				UpdateComponent(e, material);
			},
			[&](Entity e, MaterialComponent& material)
			{
				InitializeComponent(e, material);
			},
			[&](Entity e, MaterialComponent& material)
			{
				FinalizeComponent(e, material);
			})
	{
	}
	~MaterialSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, MaterialComponent& material);
	void UpdateComponent([[maybe_unused]] Entity e, MaterialComponent& material);
	void FinalizeComponent([[maybe_unused]] Entity e, MaterialComponent& material);

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	void SetBuffer(StructuredBuffer<BUFFER_DATA_MATERIAL>* buffer)
	{
		m_pIntegrationBuffer = buffer;
	}
	ResourceManager* m_pResourceManager = nullptr;
	StructuredBuffer<BUFFER_DATA_MATERIAL>* m_pIntegrationBuffer = nullptr;
};

class ParticleEmitterSystem : public ECSManager::System<ParticleComponent, EmitterComponent>
{
	friend class GameCore;
public:
	ParticleEmitterSystem():
		ECSManager::System<ParticleComponent, EmitterComponent>(
			[&](Entity e, ParticleComponent& particle, EmitterComponent& emitter)
			{
				UpdateComponent(e, particle, emitter);
			},
			[&](Entity e, ParticleComponent& particle, EmitterComponent& emitter)
			{
				InitializeComponent(e, particle, emitter);
			},
			[&](Entity e, ParticleComponent& particle, EmitterComponent& emitter)
			{
				FinalizeComponent(e, particle, emitter);
			})
	{
	}
	~ParticleEmitterSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, ParticleComponent& particle, EmitterComponent& emitter);
	void UpdateComponent([[maybe_unused]] Entity e, ParticleComponent& particle, EmitterComponent& emitter);
	void FinalizeComponent([[maybe_unused]] Entity e, ParticleComponent& particle, EmitterComponent& emitter);

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	void SetGraphicsEngine(GraphicsEngine* graphicsEngine)
	{
		m_pGraphicsEngine = graphicsEngine;
	}

	ResourceManager* m_pResourceManager = nullptr;
	GraphicsEngine* m_pGraphicsEngine = nullptr;
};

class UISpriteSystem : public ECSManager::System<UISpriteComponent>
{
	friend class GameCore;
public:
	UISpriteSystem() : 
		ECSManager::System<UISpriteComponent>(
			[&](Entity e, UISpriteComponent& uiSprite)
			{
				UpdateComponent(e, uiSprite);
			},
			[&](Entity e, UISpriteComponent& uiSprite)
			{
				InitializeComponent(e, uiSprite);
			},
			[&](Entity e, UISpriteComponent& uiSprite)
			{
				e;
				FinalizeComponent(e, uiSprite);
			})
	{
	}
	~UISpriteSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, UISpriteComponent& uiSprite);
	void UpdateComponent([[maybe_unused]] Entity e, UISpriteComponent& uiSprite);
	void FinalizeComponent([[maybe_unused]] Entity e, UISpriteComponent& uiSprite);

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	void SetBuffer(StructuredBuffer<BUFFER_DATA_UISPRITE>* buffer)
	{
		m_pIntegrationBuffer = buffer;
	}

	ResourceManager* m_pResourceManager = nullptr;
	StructuredBuffer<BUFFER_DATA_UISPRITE>* m_pIntegrationBuffer = nullptr;
};

class LightSystem : public ECSManager::System<TransformComponent, LightComponent>
{
	friend class GameCore;
public:
	LightSystem() : ECSManager::System<TransformComponent, LightComponent>(
		[&](Entity e, TransformComponent& transform, LightComponent& light)
		{
			UpdateComponent(e, transform, light);
		},
		[&](Entity e, TransformComponent& transform, LightComponent& light)
		{
			e;
			InitializeComponent(e, transform, light);
		},
		[&](Entity e, TransformComponent& transform, LightComponent& light)
		{
			e;
			FinalizeComponent(e, transform, light);
		})
	{
	}
	~LightSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, TransformComponent& transform, LightComponent& light);
	void UpdateComponent([[maybe_unused]] Entity e, TransformComponent& transform, LightComponent& light);
	void FinalizeComponent([[maybe_unused]] Entity e, [[maybe_unused]] TransformComponent& transform, [[maybe_unused]] LightComponent& light);

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}

	ResourceManager* m_pResourceManager = nullptr;
};

class AnimationSystem : public ECSManager::System<AnimationComponent>
{
	friend class GameCore;
public:
	AnimationSystem(): ECSManager::System<AnimationComponent>(
			[&](Entity e, AnimationComponent& animation)
			{
				UpdateComponent(e, animation);
			},
			[&](Entity e, AnimationComponent& animation)
			{
				InitializeComponent(e, animation);
			},
			[&](Entity e, AnimationComponent& animation)
			{
				FinalizeComponent(e, animation);
			})
	{
	}
	~AnimationSystem() = default;
private:
	void InitializeComponent([[maybe_unused]] Entity e, AnimationComponent& animation);
	void UpdateComponent([[maybe_unused]] Entity e, AnimationComponent& animation);
	void FinalizeComponent([[maybe_unused]] Entity e, AnimationComponent& animation);
	float3 CalculateValue(const std::vector<Keyframefloat3>& keyframes, const float& time);
	Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, const float& time);
	Scale CalculateValue(const std::vector<KeyframeScale>& keyframes, const float& time);
	void timeUpdate(AnimationComponent& animation, ModelData* model);
	void ApplyAnimation(AnimationComponent& animation, ModelData* model);
	void SkeletonUpdate(AnimationComponent& animation, ModelData* model);
	void SkinClusterUpdate(AnimationComponent& animation, ModelData* model);
	void ApplySkinning(AnimationComponent& animation, ModelData* model);

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	void SetGraphicsEngine(GraphicsEngine* graphicsEngine)
	{
		m_pGraphicsEngine = graphicsEngine;
	}

	ResourceManager* m_pResourceManager = nullptr;
	GraphicsEngine* m_pGraphicsEngine = nullptr;
};

class EffectEditorSystem : public ECSManager::System<EffectComponent>
{
	friend class GameCore;
public:
	EffectEditorSystem() :
		ECSManager::System<EffectComponent>(
			[&](Entity e, EffectComponent& effect)
			{
				UpdateComponent(e, effect);
			},
			[&](Entity e, EffectComponent& effect)
			{
				InitializeComponent(e, effect);
			},
			[&](Entity e, EffectComponent& effect)
			{
				FinalizeComponent(e, effect);
			})
	{
	}
	~EffectEditorSystem() = default;
	void Update() override
	{
		// いつもの処理
		ECSManager::System<EffectComponent>::Update();
		// エフェクトの初期化
		InitEffectParticle();
		// シェーダーの更新
		UpdateShader();
	}
private:
	void InitializeComponent([[maybe_unused]] Entity e, EffectComponent& effect);
	void UpdateComponent([[maybe_unused]] Entity e, EffectComponent& effect);
	void FinalizeComponent([[maybe_unused]] Entity e, EffectComponent& effect);

	void UpdateEffect(EffectComponent& effect);
	void InitEffectParticle();
	void UpdateShader();

	void SetResourceManager(ResourceManager* resourceManager)
	{
		m_pResourceManager = resourceManager;
	}
	void SetGraphicsEngine(GraphicsEngine* graphicsEngine)
	{
		m_pGraphicsEngine = graphicsEngine;
	}

	GraphicsEngine* m_pGraphicsEngine = nullptr;
	ResourceManager* m_pResourceManager = nullptr;
};
