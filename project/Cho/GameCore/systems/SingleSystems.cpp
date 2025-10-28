#include "pch.h"
#include "SingleSystems.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Graphics/GraphicsEngine/GraphicsEngine.h"
#include "GameCore/GameWorld/GameWorld.h"
#include "EngineCommand/EngineCommand.h"
#include "GameCore/Marionnette/Marionnette.h"
#include "Platform/FileSystem/FileSystem.h"
#include "OS/Windows/WinApp/WinApp.h"
#include "Core/ChoLog/ChoLog.h"
#include "Platform/Timer/Timer.h"
#include "Editor/Console/Console.h"

void TransformSystem::InitializeComponent(Entity e, TransformComponent& transform)
{
	e;
	// 初期値保存
	transform.startValue.translation = transform.position;
	transform.startValue.rotation = transform.quaternion;
	transform.startValue.scale = transform.scale;
	transform.startValue.degrees = transform.degrees;

	// 度数からラジアンに変換
	float3 radians = Theatria::Math::DegreesToRadians(transform.degrees);

	// 各軸のクオータニオンを作成
	Quaternion qx = Theatria::Math::MakeRotateAxisAngleQuaternion(float3(1.0f, 0.0f, 0.0f), radians.x);
	Quaternion qy = Theatria::Math::MakeRotateAxisAngleQuaternion(float3(0.0f, 1.0f, 0.0f), radians.y);
	Quaternion qz = Theatria::Math::MakeRotateAxisAngleQuaternion(float3(0.0f, 0.0f, 1.0f), radians.z);

	// 同時回転を累積
	transform.quaternion = qx * qy * qz;

	// 精度を維持するための正規化
	transform.quaternion.Normalize();

	// アフィン変換
	transform.matWorld = Theatria::Math::MakeAffineMatrix(transform.scale, transform.quaternion, transform.position);

	// 次のフレーム用に保存する
	transform.prePos = transform.position;
	transform.preRot = radians;
	transform.preScale = transform.scale;
}

// 優先度順に更新する
void TransformSystem::priorityUpdate()
{
	std::vector<std::pair<Entity, TransformComponent*>> transforms;

	// すべてのTransformComponentを取得
	for (auto& [arch, container] : m_pEcs->GetArchToEntities())
	{
		if ((arch & GetRequired()) == GetRequired())
		{
			for (Entity e : container.GetEntities())
			{
				if (auto* t = m_pEcs->GetComponent<TransformComponent>(e))
				{
					transforms.emplace_back(e, t);
				}
			}
		}
	}

	// 親子関係優先 + tickPriority順で安定ソート
	std::sort(transforms.begin(), transforms.end(),
		[](const auto& a, const auto& b)
		{
			const auto& aParent = a.second->parent;
			const auto& bParent = b.second->parent;

			if (aParent && bParent)
			{
				if (*aParent == b.first) return false; // b is parent of a
				if (*bParent == a.first) return true;  // a is parent of b
			}
			else if (aParent && *aParent == b.first) return false;
			else if (bParent && *bParent == a.first) return true;

			// 優先度比較
			return a.second->tickPriority < b.second->tickPriority;
		});

	// 更新実行
	for (auto& [e, t] : transforms)
	{
		UpdateComponent(e, *t);
	}
}

void TransformSystem::UpdateComponent(Entity e, TransformComponent& transform)
{
	// TransformIDがないならエラー
	if (!transform.mapID.has_value())
	{
		Log::Write(LogLevel::Assert, "TransformComponent does not have a mapID. Please initialize the TransformComponent before updating.");
	}

	// 度数からラジアンに変換
	float3 radians = Theatria::Math::DegreesToRadians(transform.degrees);
	Rigidbody3DComponent* rb3d = m_pEcs->GetComponent<Rigidbody3DComponent>(e);
	if (m_isQuaternion)
	{
		// クォータニオンを使用する場合
		if (rb3d&&rb3d->runtimeBody)
		{
			transform.quaternion = Theatria::Math::MakeQuaternionRotation(radians, transform.preRot, rb3d->quaternion);
		}
		else
		{
			transform.quaternion = Theatria::Math::MakeQuaternionRotation(radians, transform.preRot, transform.quaternion);
		}
	}
	else
	{
		// オイラー角を使用する場合
		transform.quaternion = Theatria::Math::MakeEulerRotation(radians);
	};

	// スケール行列
	transform.matScale = Theatria::Math::ScaleMatrix(transform.scale);
	// 回転行列
	float4x4 matRotation = float4x4::Identity();
	matRotation = Theatria::Math::MakeRotateMatrix(transform.quaternion);
	transform.matRotation = matRotation;
	if (transform.isBillboard)
	{
		// メインカメラのTransformを取得
		TransformComponent* cameraTransform = m_pEcs->GetComponent<TransformComponent>(m_pGameWorld->GetMainCamera()->GetHandle().entity);
		matRotation = Theatria::Math::BillboardMatrix(cameraTransform->matWorld);
	}
	// 平行移動行列
	transform.matLocal = Theatria::Math::TranslateMatrix(transform.position);
	// ワールド行列
	transform.matWorld = transform.matScale * matRotation * transform.matLocal;

	// 次のフレーム用に保存する
	transform.prePos = transform.position;
	transform.preRot = radians;
	transform.preScale = transform.scale;

	// 親があれば親のワールド行列を掛ける
	if (transform.parent.has_value())
	{
		transform.matWorld = transform.matWorld * m_pEcs->GetComponent<TransformComponent>(transform.parent.value())->matWorld;
	}

	// 物理コンポーネントがあれば、物理ボディの位置を更新
	Rigidbody2DComponent* rb = m_pEcs->GetComponent<Rigidbody2DComponent>(e);
	if (rb && rb->runtimeBody)
	{
		rb->runtimeBody->SetLinearVelocity(float2(rb->velocity.x, rb->velocity.y));
	}
	if(rb3d && rb3d->runtimeBody)
	{
		float3 linearF = float3(1.0f, 1.0f, 1.0f);
		float3 velocity = rb3d->velocity;
		float3 angularF = float3(1.0f, 1.0f, 1.0f);
		float3 angularVelocity = rb3d->angularVelocity;
		// 位置
		if(rb3d->fixedPositionX)
		{
			linearF.x = 0.0f;
			velocity.x = 0.0f;
		}
		if(rb3d->fixedPositionY)
		{
			linearF.y = 0.0f;
			velocity.y = 0.0f;
		}
		if(rb3d->fixedPositionZ)
		{
			linearF.z = 0.0f;
			velocity.z = 0.0f;
		}
		rb3d->runtimeBody->SetLinearFactor(linearF);
		rb3d->runtimeBody->SetLinearVelocity(velocity);
		// 回転
		if(rb3d->fixedRotationX)
		{
			angularF.x = 0.0f;
			angularVelocity.x = 0.0f;
		}
		if(rb3d->fixedRotationY)
		{
			angularF.y = 0.0f;
			angularVelocity.y = 0.0f;
		}
		if(rb3d->fixedRotationZ)
		{
			angularF.z = 0.0f;
			angularVelocity.z = 0.0f;
		}
		rb3d->runtimeBody->SetAngularFactor(angularF);
		rb3d->runtimeBody->SetAngularVelocity(angularVelocity);
	}

	// アニメーションコンポーネントがあればスキニングの確認
	AnimationComponent* anim = m_pEcs->GetComponent<AnimationComponent>(e);
	if (anim && anim->boneOffsetID.has_value())
	{
		transform.boneOffsetID = anim->boneOffsetID.value();
	}

	// 各行列
	transform.matLocal = Theatria::Math::TranslateMatrix(transform.position);
	transform.matRotation = Theatria::Math::MakeRotateMatrix(transform.quaternion);
	transform.matScale = Theatria::Math::ScaleMatrix(transform.scale);

	// 前方ベクトル
	transform.forward = Theatria::Math::GetForwardVectorFromMatrix(transform.matRotation);
	transform.forward.Normalize();

	// 行列の転送
	TransferMatrix(transform);
}

// 転送
void TransformSystem::TransferMatrix(TransformComponent& transform)
{
	// マップデータ更新
	BUFFER_DATA_TF data = {};
	data.matWorld = transform.matWorld;
	data.worldInverse = float4x4::Transpose((float4x4::Inverse(transform.matWorld)));
	data.rootMatrix = transform.rootMatrix;
	if (transform.materialID.has_value())
	{
		data.materialID = transform.materialID.value();
	}
	else
	{
		data.materialID = 0;
	}
	if (transform.boneOffsetID.has_value())
	{
		data.isAnimated = 1;
		data.boneOffsetStartIndex = transform.boneOffsetID.value();
	}
	else
	{
		data.isAnimated = 0;
		data.boneOffsetStartIndex = 0;
	}
	m_pIntegrationBuffer->UpdateData(data, transform.mapID.value());
}

void TransformSystem::FinalizeComponent(Entity e, TransformComponent& transform)
{
	e;
	// 初期値に戻す
	transform.position = transform.startValue.translation;
	transform.quaternion = transform.startValue.rotation;
	transform.scale = transform.startValue.scale;
	transform.degrees = transform.startValue.degrees;
}

void CameraSystem::InitializeComponent(Entity e, TransformComponent& transform, CameraComponent& camera )
{
	e; transform; camera;
}

void CameraSystem::UpdateComponent(Entity e, TransformComponent& transform, CameraComponent& camera)
{
	// 度数からラジアンに変換
	float3 radians = Theatria::Math::DegreesToRadians(transform.degrees);

	if (m_isQuaternion)
	{
		// クォータニオンを使用する場合
		transform.quaternion = Theatria::Math::MakeQuaternionRotation(radians, transform.preRot, transform.quaternion);
	}
	else
	{
		// オイラー角を使用する場合
		transform.quaternion = Theatria::Math::MakeEulerRotation(radians);
	};

	// アフィン変換
	transform.matWorld = Theatria::Math::MakeAffineMatrix(transform.scale, transform.quaternion, transform.position);

	// 次のフレーム用に保存する
	transform.prePos = transform.position;
	transform.preRot = radians;
	transform.preScale = transform.scale;

	// 親があれば親のワールド行列を掛ける
	if (transform.parent.has_value())
	{
		transform.matWorld = transform.matWorld * m_pEcs->GetComponent<TransformComponent>(transform.parent.value())->matWorld;
	}

	// 物理コンポーネントがあれば、物理ボディの位置を更新
	Rigidbody2DComponent* rb = m_pEcs->GetComponent<Rigidbody2DComponent>(e);
	if (rb && rb->runtimeBody)
	{
		rb->runtimeBody->SetLinearVelocity(float2(rb->velocity.x, rb->velocity.y));
	}

	// 各行列
	transform.matLocal = Theatria::Math::TranslateMatrix(transform.position);
	transform.matRotation = Theatria::Math::MakeRotateMatrix(transform.quaternion);
	transform.matScale = Theatria::Math::ScaleMatrix(transform.scale);

	camera.viewMatrix = float4x4::Inverse(transform.matWorld);
	camera.projectionMatrix = Theatria::Math::PerspectiveFovMatrix(camera.fovAngleY, camera.aspectRatio, camera.nearZ, camera.farZ);
	TransferMatrix(transform, camera);
}

void CameraSystem::TransferMatrix(TransformComponent& transform, CameraComponent& camera)
{
	BUFFER_DATA_VIEWPROJECTION data = {};
	data.matWorld = transform.matWorld;
	data.view = camera.viewMatrix;
	data.projection = camera.projectionMatrix;
	data.projectionInverse = float4x4::Inverse(data.projection);
	data.cameraPosition = transform.position;
	ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>* cameraBuffer = dynamic_cast<ConstantBuffer<BUFFER_DATA_VIEWPROJECTION>*>(m_pResourceManager->GetBuffer<IConstantBuffer>(camera.bufferIndex.value()));
	cameraBuffer->UpdateData(data);
}

void CameraSystem::FinalizeComponent(Entity e, TransformComponent& transform, CameraComponent& camera)
{
	e; transform; camera;
}

void ScriptSystem::InitializeComponent(Entity, ScriptComponent& script)
{
	if (!script.isActive || !script.instance) return;
	try
	{
		// スクリプトのStart関数を呼び出す
		script.startFunc();
	}
	catch (const std::exception& e)
	{
		// スクリプトのエラー処理
		Log::Write(LogLevel::Debug, "Script error: " + std::string(e.what()));
		script.isActive = false;
	}
	catch (...)
	{
		// その他のエラー処理
		Log::Write(LogLevel::Debug, "Unknown script error");
        std::string err = "Script error in " + script.scriptName;
        m_pConsole->AddColored(Console::LogColor::Red, err.c_str());
		script.isActive = false;
	}
    // fieldの上書き
    ScriptData* sd = m_pScriptContainer->GetScriptDataByName(script.scriptName);
    if (script.fields.empty())
    {
        sd->saveFields.clear();
        return;
    }
    for (const auto& saveField : sd->saveFields)
    {
        if (!script.fields.contains(saveField.first)) { continue; }
        ScriptComponent::FieldDesc& field = script.fields[saveField.first];
        if (saveField.second.type == typeid(float) && field.type == typeid(float))
        {
            *static_cast<float*>(field.ptr) = std::get<float>(saveField.second.value);
            field.minmax = saveField.second.minmax;
            field.type = saveField.second.type;
        }else if(saveField.second.type == typeid(int) && field.type == typeid(int))
        {
            *static_cast<int*>(field.ptr) = std::get<int>(saveField.second.value);
            field.minmax = saveField.second.minmax;
            field.type = saveField.second.type;
        }
        else if (saveField.second.type == typeid(bool) && field.type == typeid(bool))
        {
            *static_cast<bool*>(field.ptr) = std::get<bool>(saveField.second.value);
            field.minmax = saveField.second.minmax;
            field.type = saveField.second.type;
        }
        else if (saveField.second.type == typeid(float3) && field.type == typeid(float3))
        {
            *static_cast<float3*>(field.ptr) = std::get<float3>(saveField.second.value);
            field.minmax = saveField.second.minmax;
            field.type = saveField.second.type;
        }
        else
        {
            continue;
        }
    }
    sd->saveFields.clear();
}

void ScriptSystem::UpdateComponent(Entity, ScriptComponent& script)
{
	if (!script.isActive || !script.instance) return;
	try
	{
		// スクリプトのUpdate関数を呼び出す
		script.updateFunc();
	}
	catch (const std::exception& e)
	{
		// スクリプトのエラー処理
		Log::Write(LogLevel::Debug, "Script error: " + std::string(e.what()));
        std::string err = "Script error in " + script.scriptName;
        std::string errInfo = std::string(e.what());
        m_pConsole->AddColored(Console::LogColor::Red, err.c_str());
        m_pConsole->AddColored(Console::LogColor::Red, errInfo.c_str());
		script.isActive = false;
	}
	catch (...)
	{
		// その他のエラー処理
		Log::Write(LogLevel::Debug, "Unknown script error");
        std::string err = "Script error in " + script.scriptName;
        m_pConsole->AddColored(Console::LogColor::Red, err.c_str());
		script.isActive = false;
	}
}

void ScriptSystem::FinalizeComponent(Entity e, ScriptComponent& script)
{
	e;
	if (!script.isActive || !script.instance) return;
    // fieldの保存
    ScriptData* sd = m_pScriptContainer->GetScriptDataByName(script.scriptName);
    if (script.fields.empty()) { script.fields.clear(); return; }
    for (const auto& field : script.fields)
    {
        ScriptComponent::FieldVal& save = sd->saveFields[field.first];
        if (field.second.type == typeid(float))
        {
            save.value = *static_cast<float*>(field.second.ptr);
            save.minmax = field.second.minmax;
            save.type = field.second.type;
        }
        else if (field.second.type == typeid(int))
        {
            save.value = *static_cast<int*>(field.second.ptr);
            save.minmax = field.second.minmax;
            save.type = field.second.type;
        }
        else if (field.second.type == typeid(bool))
        {
            save.value = *static_cast<bool*>(field.second.ptr);
            save.minmax = field.second.minmax;
            save.type = field.second.type;
        }
        else if (field.second.type == typeid(float3))
        {
            save.value = *static_cast<float3*>(field.second.ptr);
            save.minmax = field.second.minmax;
            save.type = field.second.type;
        }
        else
        {
            continue;
        }
    }
    script.fields.clear();
	try
	{
		// スクリプトのCleanup関数を呼び出す
		script.cleanupFunc();
		script.instance = nullptr;
	}
	catch (const std::exception& e)
	{
		// スクリプトのエラー処理
		Log::Write(LogLevel::Debug, "Script error: " + std::string(e.what()));
	}
	catch (...)
	{
		// その他のエラー処理
		Log::Write(LogLevel::Debug, "Unknown script error");
	}
	script.isActive = false;
}

void ScriptSystem::AwakeComponent(Entity e, ScriptComponent& script)
{
	if (script.scriptName.empty())
	{
		script.isActive = false;
		return;
	}
	std::string funcName = "Create" + script.scriptName + "Script";
	funcName.erase(std::remove_if(funcName.begin(), funcName.end(), ::isspace), funcName.end());
	// CreateScript関数を取得
	typedef Marionnette* (*CreateScriptFunc)(GameObject&);
	CreateScriptFunc createScript = (CreateScriptFunc)GetProcAddress(theatria::FileSystem::ScriptProject::m_DllHandle, funcName.c_str());
	if (!createScript)
	{
		script.isActive = false;
		return;
	}
	// スクリプトを生成
	GameObject* object = m_pGameWorld->GetGameObject(e);
	Marionnette* scriptInstance = createScript(*object);
	if (!scriptInstance)
	{
		script.isActive = false;
		return;
	}
	// Handleを取得
	script.objectHandle = object->GetHandle();
	// ECSをセット
	scriptInstance->SetECSPtr(m_pEcs);
	// スクリプトのStart関数とUpdate関数をラップ
	script.startFunc = [scriptInstance]() {
		scriptInstance->Start();
		};
	script.updateFunc = [scriptInstance]() {
		scriptInstance->Update();
		};
	// インスタンスの解放用のクロージャを設定
	script.cleanupFunc = [scriptInstance, this]() {
		delete scriptInstance;
		};
	// 衝突関数をラップ
	script.onCollisionEnterFunc = [scriptInstance](GameObject& other) {
		scriptInstance->OnCollisionEnter(other);
		};
	script.onCollisionStayFunc = [scriptInstance](GameObject& other) {
		scriptInstance->OnCollisionStay(other);
		};
	script.onCollisionExitFunc = [scriptInstance](GameObject& other) {
		scriptInstance->OnCollisionExit(other);
		};
	script.instance = scriptInstance;
	script.isActive = true;
}

void Rigidbody2DSystem::InitializeComponent(Entity e, TransformComponent& transform, Rigidbody2DComponent& rb)
{
	e;
	physics::d2::Id2BodyDef bodyDef;
	bodyDef.userData = static_cast<void*>(&rb.selfEntity.value());
	bodyDef.type = rb.bodyType;
	bodyDef.gravityScale = rb.gravityScale;
	bodyDef.fixedRotation = rb.fixedRotation;
	bodyDef.position = float2(transform.position.x, transform.position.y);
	float angleZ = Theatria::Math::DegreesToRadians(transform.degrees).z;
	bodyDef.angle = angleZ;
	rb.runtimeBody = m_World->CreateBody(bodyDef);
	rb.runtimeBody->SetAwake(true);
	rb.velocity.Initialize();

	// Transformと同期（optional）
	transform.position.x = rb.runtimeBody->GetPosition().x;
	transform.position.y = rb.runtimeBody->GetPosition().y;
}

void Rigidbody2DSystem::StepSimulation()
{
	float timeStep = Timer::GetDeltaTime();
	/*constexpr int velocityIterations = 6;
	constexpr int positionIterations = 2;*/
	m_World->Step(timeStep, 4);
}

void Rigidbody2DSystem::UpdateComponent(Entity e, TransformComponent& transform, Rigidbody2DComponent& rb)
{
	e;
	if (!rb.runtimeBody) return;

	if (rb.requestedPosition)
	{
		if (rb.fixedRotation)
		{
			rb.runtimeBody->SetTransform(*rb.requestedPosition, rb.runtimeBody->GetAngle());
		}
		else
		{
			rb.runtimeBody->SetTransform(*rb.requestedPosition, Theatria::Math::DegreesToRadians(transform.degrees).z);
		}
		rb.requestedPosition.reset();
	}
	const float2& pos = rb.runtimeBody->GetPosition();
	transform.position.x = pos.x;
	transform.position.y = pos.y;

	float2 velocity = rb.runtimeBody->GetLinearVelocity();
	rb.velocity.x = velocity.x;
	rb.velocity.y = velocity.y;

	float3 radians = {};
	if (!rb.fixedRotation)
	{
		radians.z = rb.runtimeBody->GetAngle(); // radians
	}
	else
	{
		radians.z = Theatria::Math::DegreesToRadians(transform.degrees).z;
	}
	float3 degrees = Theatria::Math::RadiansToDegrees(radians);
	transform.degrees.z = degrees.z;
}

void Rigidbody2DSystem::Reset(Rigidbody2DComponent& rb)
{
	// Bodyがあるなら削除
	if (rb.runtimeBody)
	{
		rb.runtimeBody->DestroyShape();
		m_World->DestroyBody(rb.runtimeBody);
	}
	rb.runtimeBody = nullptr;
	rb.isCollisionStay = false;
	rb.otherEntity.reset();
}

void Rigidbody2DSystem::FinalizeComponent(Entity e, TransformComponent& transform, Rigidbody2DComponent& rb)
{
	transform; e;
	Reset(rb);
}

void CollisionSystem::CollisionStay(ScriptComponent& script, Rigidbody2DComponent& rb)
{
	if (script.isActive && rb.isCollisionStay && rb.otherEntity)
	{
		// 相手のゲームオブジェクトを取得
		GameObject* other = m_pGameWorld->GetGameObject(rb.otherEntity.value());
		if (!other) { return; }
		if (!rb.isActive) { return; }
		script.onCollisionStayFunc(*other);
	}
}

void Collider2DSystem::InitializeComponent(Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box)
{
	transform;
	e;
	if (!rb.runtimeBody&&box.runtimeShape) return;

	
	physics::d2::Id2Polygon polygonShape;
	polygonShape.SetSize(box.width, box.height);
	box.preHeight = box.height;
	box.preWidth = box.width;
	// 面積を計算
	float area = box.width * box.height;
	physics::d2::Id2ShapeDef shapeDef;
	shapeDef.density = rb.mass / area;
	shapeDef.friction = box.friction;
	shapeDef.restitution = box.restitution;
	shapeDef.isSensor = box.isSensor;
	box.runtimeShape = rb.runtimeBody->CreateShape(&shapeDef, &polygonShape);
}

void Collider2DSystem::UpdateComponent(Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box)
{
	e;
	transform;
	// サイズの変更があった場合、フィクスチャを再作成
	if (!box.runtimeShape) { return; }
	if (box.width != box.preWidth || box.height != box.preHeight)
	{
		rb.runtimeBody->DestroyShape();
		rb.runtimeBody = nullptr;
		physics::d2::Id2Polygon polygonShape;
		polygonShape.MakeBox(box.width * 0.5f, box.height * 0.5f);
		box.preHeight = box.height;
		box.preWidth = box.width;
		// 面積を計算
		float area = box.width * box.height;
		physics::d2::Id2ShapeDef shapeDef;
		shapeDef.density = rb.mass / area;
		shapeDef.friction = box.friction;
		shapeDef.restitution = box.restitution;
		shapeDef.isSensor = box.isSensor;
		box.runtimeShape = rb.runtimeBody->CreateShape(&shapeDef, &polygonShape);
	}
}

void Collider2DSystem::FinalizeComponent(Entity e, TransformComponent& transform, Rigidbody2DComponent& rb, BoxCollider2DComponent& box)
{
	e; transform; rb; box;
	if (box.runtimeShape)
	{
		box.runtimeShape = nullptr;
	}
}

void MaterialSystem::InitializeComponent(Entity e, MaterialComponent& material)
{
	e; material;
}

void MaterialSystem::UpdateComponent(Entity e, MaterialComponent& material)
{
	e;
	BUFFER_DATA_MATERIAL data = {};
	data.color = material.color;
	data.enableLighting = material.enableLighting;
	data.cubeUVScale = material.cubeUVScale;
	if (material.enableTexture)
	{
		data.enableTexture = true;
		if (!material.textureName.empty() && m_pResourceManager->GetTextureManager()->IsTextureExist(material.textureName))
		{
			if (material.textureID.has_value())
			{
				data.textureId = material.textureID.value();
			}
			else
			{
				data.textureId = m_pResourceManager->GetTextureManager()->GetTextureID(material.textureName);
			}
		}
		else
		{
			data.textureId = 0;
		}
		if(!material.cubeTextureName.empty() && m_pResourceManager->GetTextureManager()->IsTextureExist(material.cubeTextureName))
		{
			if (material.cubeTextureID.has_value())
			{
				data.cubeTextureId = material.cubeTextureID.value();
			}
			else
			{
				data.cubeTextureId = m_pResourceManager->GetTextureManager()->GetTextureID(material.cubeTextureName);
			}
		}
		else
		{
			data.cubeTextureId = 0;
		}
	}
	else
	{
		data.enableTexture = 0;
		data.textureId = 0;
	}
	data.matUV = material.matUV.Identity();
	data.shininess = material.shininess;
	data.uvFlipY = material.uvFlipY;
	m_pIntegrationBuffer->UpdateData(data, material.mapID.value());
}

void MaterialSystem::FinalizeComponent(Entity e, MaterialComponent& material)
{
	e; material;
}

void ParticleEmitterSystem::InitializeComponent(Entity e, ParticleComponent& particle, EmitterComponent& emitter)
{
	e; emitter;
	particle.time = 0.0f;

	// CSで初期化
	// コンテキスト取得
	CommandContext* context = m_pGraphicsEngine->GetCommandContext();
	// コマンドリスト開始
	m_pGraphicsEngine->BeginCommandContext(context);
	// パイプラインセット
	context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetParticleInitPSO().pso.Get());
	// ルートシグネチャセット
	context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetParticleInitPSO().rootSignature.Get());
	// パーティクルバッファをセット
	IRWStructuredBuffer* particleBuffer = m_pResourceManager->GetBuffer<IRWStructuredBuffer>(particle.bufferIndex);
	context->SetComputeRootDescriptorTable(0, particleBuffer,ViewType::UnorderedAccessView);
	// フリーリストバッファをセット
	IRWStructuredBuffer* freeListBuffer = m_pResourceManager->GetBuffer<IRWStructuredBuffer>(particle.freeListBufferIndex);
	context->SetComputeRootDescriptorTable(1, freeListBuffer, ViewType::UnorderedAccessView);
	// カウンターバッファをセット
	context->SetComputeRootUnorderedAccessView(2, freeListBuffer);
	// Dispatch
	context->Dispatch(1, 1, 1);
	// コマンドリストをクローズ
	m_pGraphicsEngine->EndCommandContext(context, QueueType::Compute);
	// GPUの処理が終わるまで待機
	m_pGraphicsEngine->WaitForGPU(QueueType::Compute);
}

void ParticleEmitterSystem::UpdateComponent(Entity e, ParticleComponent& particle, EmitterComponent& emitter)
{
	TransformComponent* transform = m_pEcs->GetComponent<TransformComponent>(e);
	emitter.frequencyTime += DeltaTime();

	// 射出間隔を上回ったら射出許可を出して時間を調整
	//if (emitter.frequency <= emitter.frequencyTime)
	//{
	//	emitter.frequencyTime = 0.0f;
	//	emitter.emit = 1;
	//} else
	//{
	//	// 射出間隔を上回ってないので、許可は出せない
	//	emitter.emit = 1;
	//}
	{
		BUFFER_DATA_EMITTER data = {};
		data.lifeTime = emitter.lifeTime;
		data.position = emitter.position;
		data.position.value.x.median = transform->position.x;
		data.position.value.y.median = transform->position.y;
		data.position.value.z.median = transform->position.z;
		data.rotation = emitter.rotation;
		data.scale = emitter.scale;
		data.frequency = emitter.frequency;
		data.frequencyTime = emitter.frequencyTime;
		data.emit = emitter.emit;
		data.emitCount = emitter.emitCount;
		data.isFadeOut = emitter.isFadeOut;
		data.isBillboard = emitter.isBillboard;
		if (transform->materialID.has_value())
		{
			data.materialID = transform->materialID.value();
		}
		else
		{
			data.materialID = 0;
		}
		StructuredBuffer<BUFFER_DATA_EMITTER>* buffer = dynamic_cast<StructuredBuffer<BUFFER_DATA_EMITTER>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(emitter.bufferIndex));
		buffer->UpdateData(data, 0);
		emitter.emit = false;
	}

	particle.time += DeltaTime();
	particle.deltaTime = DeltaTime();

	{
		BUFFER_DATA_PARTICLE_PERFRAME data = {};
		data.time = particle.time;
		data.deltaTime = particle.deltaTime;
		ConstantBuffer<BUFFER_DATA_PARTICLE_PERFRAME>* buffer = dynamic_cast<ConstantBuffer<BUFFER_DATA_PARTICLE_PERFRAME>*>(m_pResourceManager->GetBuffer<IConstantBuffer>(particle.perFrameBufferIndex));
		buffer->UpdateData(data);
	}
	// Emit
	CommandContext* context = m_pGraphicsEngine->GetCommandContext();
	m_pGraphicsEngine->BeginCommandContext(context);
	// パイプラインセット
	context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetParticleEmitPSO().pso.Get());
	// ルートシグネチャセット
	context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetParticleEmitPSO().rootSignature.Get());
	// パーティクルバッファをセット
	IRWStructuredBuffer* particleBuffer = m_pResourceManager->GetBuffer<IRWStructuredBuffer>(particle.bufferIndex);
	context->SetComputeRootDescriptorTable(0, particleBuffer, ViewType::UnorderedAccessView);
	// エミッターバッファをセット
	IStructuredBuffer* emitterBuffer = m_pResourceManager->GetBuffer<IStructuredBuffer>(emitter.bufferIndex);
	context->SetComputeRootDescriptorTable(1, emitterBuffer, ViewType::ShaderResourceView);
	// PerFrameバッファをセット
	IConstantBuffer* perFrameBuffer = m_pResourceManager->GetBuffer<IConstantBuffer>(particle.perFrameBufferIndex);
	context->SetComputeRootConstantBufferView(2, perFrameBuffer);
	// フリーリストインデックスバッファをセット
	//IRWStructuredBuffer* freeListIndexBuffer = m_pResourceManager->GetBuffer<IRWStructuredBuffer>(particle.freeListIndexBufferIndex);
	//context->SetComputeRootDescriptorTable(3, freeListIndexBuffer->GetUAVGpuHandle());
	// フリーリストバッファをセット
	IRWStructuredBuffer* freeListBuffer = m_pResourceManager->GetBuffer<IRWStructuredBuffer>(particle.freeListBufferIndex);
	context->SetComputeRootDescriptorTable(3, freeListBuffer, ViewType::UnorderedAccessView);
	// Dispatch
	context->Dispatch(1, 1, 1);

	// EmitとUpdateの並列を阻止
	context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, particleBuffer->GetResource());
	//context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, freeListIndexBuffer->GetResource());
	context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, freeListBuffer->GetResource());

	// Update
	// パイプラインセット
	context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetParticleUpdatePSO().pso.Get());
	// ルートシグネチャセット
	context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetParticleUpdatePSO().rootSignature.Get());
	// パーティクルバッファをセット
	context->SetComputeRootDescriptorTable(0, particleBuffer, ViewType::UnorderedAccessView);
	// PerFrameバッファをセット
	context->SetComputeRootConstantBufferView(1, perFrameBuffer);
	// フリーリストインデックスバッファをセット
	//context->SetComputeRootDescriptorTable(2, freeListIndexBuffer->GetUAVGpuHandle());
	// フリーリストバッファをセット
	context->SetComputeRootDescriptorTable(2, freeListBuffer, ViewType::UnorderedAccessView);
	// Dispatch
	context->Dispatch(1, 1, 1);

	// クローズ
	m_pGraphicsEngine->EndCommandContext(context, QueueType::Compute);
	// GPUの処理が終わるまで待機
	m_pGraphicsEngine->WaitForGPU(QueueType::Compute);
}

void ParticleEmitterSystem::FinalizeComponent(Entity e, ParticleComponent& particle, EmitterComponent& emitter)
{
	e; particle; emitter;
}

void UISpriteSystem::InitializeComponent(Entity e, UISpriteComponent& uiSprite)
{
	e; uiSprite;
}

void UISpriteSystem::UpdateComponent(Entity e, UISpriteComponent& uiSprite)
{
	e;
	// UIの更新
	float left = 0.0f - uiSprite.anchorPoint.x;
	float right = uiSprite.size.x - uiSprite.anchorPoint.x;
	float top = 0.0f - uiSprite.anchorPoint.y;
	float bottom = uiSprite.size.y - uiSprite.anchorPoint.y;

	float tex_left = uiSprite.textureLeftTop.x / uiSprite.size.x;
	float tex_right = (uiSprite.textureLeftTop.x + uiSprite.size.x) / uiSprite.size.x;
	float tex_top = uiSprite.textureLeftTop.y / uiSprite.size.y;
	float tex_bottom = (uiSprite.textureLeftTop.y + uiSprite.size.y) / uiSprite.size.y;

	//uiSprite.scale = uiSprite.size;
	float3 scale = float3(uiSprite.scale.x, uiSprite.scale.y, 1.0f);
	float3 rotation = float3(0.0f, 0.0f, uiSprite.rotation);
	float3 translation = float3(uiSprite.position.x, uiSprite.position.y, 0.0f);
	float4x4 worldMatrixSprite = Theatria::Math::MakeAffineMatrix(scale, rotation, translation);

	float4x4 viewMatrixSprite = float4x4::Identity();

	float4x4 projectionMatrixSprite = Theatria::Math::OrthographicMatrix(0.0f, 0.0f,
		static_cast<float>(WinApp::GetWindowWidth()), static_cast<float>(WinApp::GetWindowHeight()),
		0.0f, 100.0f
	);

    float4x4 worldViewProjectionMatrixSprite = viewMatrixSprite * projectionMatrixSprite * worldMatrixSprite;

	uiSprite.matWorld = worldViewProjectionMatrixSprite;

	//uiSprite.material.matUV = MakeAffineMatrix(float3(uiSprite.uvScale.x, uiSprite.uvScale.y, 1.0f), float3(0.0f, 0.0f, uiSprite.uvRot), float3(uiSprite.uvPos.x, uiSprite.uvPos.y, 0.0f));

	//uiSprite.constData->matWorld = uiSprite.matWorld;

	BUFFER_DATA_UISPRITE data = {};
	data.matWorld = uiSprite.matWorld;
	data.left = left;
	data.right = right;
	data.top = top;
	data.bottom = bottom;
	data.tex_left = tex_left;
	data.tex_right = tex_right;
	data.tex_top = tex_top;
	data.tex_bottom = tex_bottom;
	MaterialComponent* material = m_pEcs->GetComponent<MaterialComponent>(e);
	if (material && material->mapID)
	{
		data.materialID = material->mapID.value();
	}
	else
	{
		data.materialID = 0;
	}
	m_pIntegrationBuffer->UpdateData(data, uiSprite.mapID.value());
}

void UISpriteSystem::FinalizeComponent(Entity e, UISpriteComponent& uiSprite)
{
	e; uiSprite;
}

void LightSystem::InitializeComponent(Entity e, TransformComponent& transform, LightComponent& light)
{
	e; transform; light;
}

void LightSystem::UpdateComponent(Entity e, TransformComponent& transform, LightComponent& light)
{
	e;
	float3 direction = Theatria::Math::TransformDirection(float3(0.0f, 0.0f, 1.0f), Theatria::Math::RotateXYZMatrix(Theatria::Math::DegreesToRadians(transform.degrees)));
	//direction.Normalize();
	// ライトのワールド行列を転送
	BUFFER_DATA_LIGHT& data = m_pResourceManager->GetLightBuffer()->GetData();
	uint32_t i = light.mapID.value();
	data.lightData[i].color = light.color;
	data.lightData[i].direction = direction;
	data.lightData[i].intensity = light.intensity;
	data.lightData[i].range = light.range;
	data.lightData[i].decay = light.decay;
	data.lightData[i].spotAngle = light.spotAngle;
	data.lightData[i].spotFalloffStart = light.spotFalloffStart;
	data.lightData[i].type = static_cast<uint32_t>(light.type);
	data.lightData[i].active = light.active;
	data.lightData[i].transformMapID = transform.mapID.value();
	// 参照に書き込んでいるので転送は不要
}

void LightSystem::FinalizeComponent(Entity e, TransformComponent& transform, LightComponent& light)
{
	e; transform; light;
}

void AnimationSystem::InitializeComponent(Entity e, AnimationComponent& animation)
{
	e; animation;
}

void AnimationSystem::UpdateComponent(Entity e, AnimationComponent& animation)
{
	e;
	ModelData* model = m_pResourceManager->GetModelManager()->GetModelData(animation.modelName);
	timeUpdate(animation, model);
}

void AnimationSystem::FinalizeComponent(Entity e, AnimationComponent& animation)
{
	e; animation;
}

Scale AnimationSystem::CalculateValue(const std::vector<KeyframeScale>& keyframes, const float& time)
{
	assert(!keyframes.empty());// キーがないものは返す値がわからないのでだめ
	if (keyframes.size() == 1 || time <= keyframes[0].time)
	{// キーが一つか、時刻がキーフレーム前なら最初の値とする
		return keyframes[0].value;
	}
	for (size_t index = 0; index < keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;
		// indexとnextIndexの二つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
		{
			// 範囲内を補間する
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Scale::Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	// ここまで来た場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return (*keyframes.rbegin()).value;
}

void AnimationSystem::timeUpdate(AnimationComponent& animation, ModelData* model)
{
	if (animation.isRun)
	{
		// animationIndexが変更された場合、遷移を開始する
		if (animation.prevAnimationIndex != animation.animationIndex)
		{
			animation.transitionIndex = animation.prevAnimationIndex;
			animation.transition = true;
			animation.transitionTime = 0.0f;
		}

		// 遷移が終了した場合、遷移フラグをfalseにする
		if (animation.lerpTime >= 1.0f)
		{
			animation.transition = false;
			animation.lerpTime = 0.0f;
			animation.time = animation.transitionTime;
			animation.transitionTime = 0.0f;
		}

		/*エンジンのデルタタイムにする*/
		static float deltaTime = 1.0f / 60.0f;

		// 遷移中の場合、遷移時間を更新
		if (animation.transition)
		{
			animation.transitionTime += deltaTime;//DeltaTime();
			animation.lerpTime = animation.transitionTime / animation.transitionDuration;
			animation.lerpTime = std::clamp(animation.lerpTime, 0.0f, 1.0f);
		}

		// 時間更新
		animation.isEnd = false;
		animation.time += deltaTime;//DeltaTime();
		if (animation.time >= model->animations[animation.animationIndex].duration)
		{
			animation.isEnd = true;
			if (!animation.isRestart)
			{
				animation.isRun = false;
			}
		}
		else
		{
			animation.isEnd = false;
		}
		if (!animation.isRestart)
		{
			animation.time;
		}
		else
		{
			animation.time = std::fmod(animation.time, model->animations[animation.animationIndex].duration);
		}
	}
	// アニメーション更新
	ApplyAnimation(animation, model);
	if (model->isBone)
	{
		SkeletonUpdate(animation, model);
		SkinClusterUpdate(animation, model);
		//ApplySkinning(animation, model);
	}
	animation.prevAnimationIndex = animation.animationIndex;
}

void AnimationSystem::ApplyAnimation(AnimationComponent& animation, ModelData* model)
{
	for (Joint& joint : animation.skeleton->joints)
	{
		// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文
		if (auto it = model->animations[animation.animationIndex].nodeAnimations.find(joint.name); it != model->animations[animation.animationIndex].nodeAnimations.end())
		{
			const NodeAnimation& rootNodeAnimation = (*it).second;
			if (animation.transition)
			{
				if (auto it2 = model->animations[animation.transitionIndex].nodeAnimations.find(joint.name); it2 != model->animations[animation.transitionIndex].nodeAnimations.end())
				{
					const NodeAnimation& rootNodeAnimation2 = (*it2).second;
					float3 startTranslate = CalculateValue(rootNodeAnimation2.translate.keyframes, animation.time);
					Quaternion startRotate = CalculateValue(rootNodeAnimation2.rotate.keyframes, animation.time);
					Scale startScale = CalculateValue(rootNodeAnimation2.scale.keyframes, animation.time);
					float3 endTranslate = CalculateValue(rootNodeAnimation.translate.keyframes, animation.transitionTime);
					Quaternion endRotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animation.transitionTime);
					Scale endScale = CalculateValue(rootNodeAnimation.scale.keyframes, animation.transitionTime);
					joint.transform.translation = float3::Lerp(startTranslate, endTranslate, animation.lerpTime);
					joint.transform.rotation = Quaternion::Slerp(startRotate, endRotate, animation.lerpTime);
					joint.transform.scale = Scale::Lerp(startScale, endScale, animation.lerpTime);
				}
			}
			else
			{
				joint.transform.translation = CalculateValue(rootNodeAnimation.translate.keyframes, animation.time);
				joint.transform.rotation = CalculateValue(rootNodeAnimation.rotate.keyframes, animation.time);
				joint.transform.scale = CalculateValue(rootNodeAnimation.scale.keyframes, animation.time);
			}
		}
	}
}

void AnimationSystem::SkeletonUpdate(AnimationComponent& animation, ModelData* model)
{
	model;
	// すべてのJointを更新。親が若いので通常ループで処理可能になっている
	for (Joint& joint : animation.skeleton->joints)
	{
		joint.localMatrix = Theatria::Math::MakeAffineMatrix(joint.transform.scale, joint.transform.rotation, joint.transform.translation);
		if (joint.parent)
		{
			joint.skeletonSpaceMatrix = joint.localMatrix * animation.skeleton->joints[*joint.parent].skeletonSpaceMatrix;
		}
		else
		{// 親がいないのでlocalMatrixとskeletonSpaceMatrixは一致する
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void AnimationSystem::SkinClusterUpdate(AnimationComponent& animation, ModelData* model)
{
	StructuredBuffer<ConstBufferDataWell>* paletteBuffer = dynamic_cast<StructuredBuffer<ConstBufferDataWell>*>(m_pResourceManager->GetBuffer<IStructuredBuffer>(model->boneMatrixBufferIndex));
	for (uint32_t jointIndex = 0; jointIndex < model->skeleton.joints.size(); ++jointIndex)
	{
		for (int i = 0;i<animation.skinClusters.size();i++)
		{
			assert(jointIndex < model->meshes[i].skinCluster.inverseBindPoseMatrices.size());
			uint32_t offset = static_cast<uint32_t>(model->skeleton.joints.size() * animation.boneOffsetID.value());
			ConstBufferDataWell data = {};
			data.skeletonSpaceMatrix =
				animation.skinClusters[i].inverseBindPoseMatrices[jointIndex] * animation.skeleton->joints[jointIndex].skeletonSpaceMatrix;
			data.skeletonSpaceInverseTransposeMatrix =
                float4x4::Transpose(float4x4::Inverse(data.skeletonSpaceMatrix));
			paletteBuffer->UpdateData(data, jointIndex + offset);
		}
	}
}

void AnimationSystem::ApplySkinning(AnimationComponent& animation, ModelData* model)
{
	animation;
	model;
	//// コマンドリストの取得
	//CommandContext* context = m_pGraphicsEngine->GetCommandContext();
	//m_pGraphicsEngine->BeginCommandContext(context);
	//// スキニングOutputリソースを遷移
	//IVertexBuffer* skinningBuffer = m_pResourceManager->GetBuffer<IVertexBuffer>(animation.skinningBufferIndex);
	//context->BarrierTransition(skinningBuffer->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//// パイプラインセット
	//context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetSkinningPSO().pso.Get());
	//// ルートシグネチャセット
	//context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetSkinningPSO().rootSignature.Get());
	//// パレットデータをセット
	//IStructuredBuffer* paletteBuffer = m_pResourceManager->GetBuffer<IStructuredBuffer>(animation.paletteBufferIndex);
	//context->SetComputeRootDescriptorTable(0, paletteBuffer->GetSRVGpuHandle());
	//// メッシュデータをセット
	//IVertexBuffer* vertexBuffer = m_pResourceManager->GetBuffer<IVertexBuffer>(model->meshes[0].vertexBufferIndex);
	//context->SetComputeRootDescriptorTable(1, vertexBuffer->GetSRVGpuHandle());
	//// 影響データをセット
	//IStructuredBuffer* influenceBuffer = m_pResourceManager->GetBuffer<IStructuredBuffer>(animation.influenceBufferIndex);
	//context->SetComputeRootDescriptorTable(2, influenceBuffer->GetSRVGpuHandle());
	//// スキニングデータをセット
	//context->SetComputeRootDescriptorTable(3, skinningBuffer->GetUAVGpuHandle());
	//// Skinning用情報
	//IConstantBuffer* infoBuffer = m_pResourceManager->GetBuffer<IConstantBuffer>(model->meshes[0].skinInfoBufferIndex);
	//context->SetComputeRootConstantBufferView(4, infoBuffer->GetResource()->GetGPUVirtualAddress());
	//// Dispatch
	//context->Dispatch(static_cast<UINT>(model->meshes[0].vertices.size() + 1023) / 1024, 1, 1);
	//// スキニングOutputリソースを元に戻す
	//context->BarrierTransition(skinningBuffer->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	//// クローズ
	//m_pGraphicsEngine->EndCommandContext(context, QueueType::Compute);
	//// GPUの処理が終わるまで待機
	//m_pGraphicsEngine->WaitForGPU(QueueType::Compute);
}

Quaternion AnimationSystem::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, const float& time)
{
	assert(!keyframes.empty());// キーがないものは返す値がわからないのでだめ
	if (keyframes.size() == 1 || time <= keyframes[0].time)
	{// キーが一つか、時刻がキーフレーム前なら最初の値とする
		return keyframes[0].value;
	}
	for (size_t index = 0; index < keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;
		// indexとnextIndexの二つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
		{
			// 範囲内を補間する
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Quaternion::Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	// ここまで来た場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return (*keyframes.rbegin()).value;
}

float3 AnimationSystem::CalculateValue(const std::vector<Keyframefloat3>& keyframes, const float& time)
{
	assert(!keyframes.empty());// キーがないものは返す値がわからないのでだめ
	if (keyframes.size() == 1 || time <= keyframes[0].time)
	{// キーが一つか、時刻がキーフレーム前なら最初の値とする
		return keyframes[0].value;
	}
	for (size_t index = 0; index < keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;
		// indexとnextIndexの二つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
		{
			// 範囲内を補間する
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return float3::Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	// ここまで来た場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return (*keyframes.rbegin()).value;
}

void EffectEditorSystem::InitializeComponent(Entity e, EffectComponent& effect)
{
	e; effect;
}

void EffectEditorSystem::UpdateComponent(Entity e, EffectComponent& effect)
{
	e;
	// EditorはTimeBaseでの更新

	// エフェクトの時間を更新
	effect.root.second.time.deltaTime = DeltaTime();
	if (effect.isRun)
	{
		effect.root.second.time.elapsedTime++;
	}
	if (effect.root.second.time.elapsedTime > effect.root.second.time.duration)
	{
		if (effect.isLoop)
		{
			// ループする場合は時間をリセット
			effect.root.second.time.elapsedTime = 0.0f;
		}
		else
		{
			// ループしない場合は停止
			effect.isRun = false;
			return;
		}
	}

	// バッファに転送
	StructuredBuffer<EffectRoot>* rootBuffer = dynamic_cast<StructuredBuffer<EffectRoot>*>(m_pResourceManager->GetIntegrationBuffer(IntegrationDataType::EffectRootInt));
	StructuredBuffer<EffectNode>* nodeBuffer = dynamic_cast<StructuredBuffer<EffectNode>*>(m_pResourceManager->GetIntegrationBuffer(IntegrationDataType::EffectNodeInt));
	StructuredBuffer<EffectSprite>* spriteBuffer = dynamic_cast<StructuredBuffer<EffectSprite>*>(m_pResourceManager->GetIntegrationBuffer(IntegrationDataType::EffectSpriteInt));

	// EffectRootを更新
	EffectRoot root;
	root.globalSeed = effect.root.second.globalSeed;
	root.nodeCount = effect.root.second.nodeCount;
	root.time = effect.root.second.time;
	uint32_t i = 0;
	for (const auto& node : effect.root.second.nodes)
	{
		root.nodeID[i] = node.nodeID;
		i++;
		// Nodeを更新
		EffectNode nodeData;
		nodeData.common = node.common;
		nodeData.position = node.position;
		nodeData.rotation = node.rotation;
		nodeData.scale = node.scale;
		nodeData.drawCommon = node.drawCommon;
		nodeData.draw = node.draw;
		nodeData.isRootParent = node.isRootParent;
		nodeData.parentIndex = node.parentIndex;
		nodeBuffer->UpdateData(nodeData, node.nodeID);
		EFFECT_MESH_TYPE meshType = static_cast<EFFECT_MESH_TYPE>(node.draw.meshType);
		switch (meshType)
		{
		case EFFECT_MESH_TYPE::NONE:
			break;
		case EFFECT_MESH_TYPE::SPRITE: {
			// スプライトの場合はスプライトバッファに転送
			EffectSprite spriteData;
			const EffectSprite* sprite = std::get_if<EffectSprite>(&node.drawMesh);
			spriteData = *sprite;
			spriteBuffer->UpdateData(spriteData, node.draw.meshDataIndex);
			break;
		}
		case EFFECT_MESH_TYPE::RIBBON:
			break;
		case EFFECT_MESH_TYPE::TRAIL:
			break;
		case EFFECT_MESH_TYPE::RING:
			break;
		case EFFECT_MESH_TYPE::MODEL:
			break;
		case EFFECT_MESH_TYPE::CUBE:
			break;
		case EFFECT_MESH_TYPE::SPHERE:
			break;
		default:
			break;
		}
	}
	rootBuffer->UpdateData(root, effect.root.first);
}

void EffectEditorSystem::FinalizeComponent(Entity e, EffectComponent& effect)
{
	e; effect;
}

void EffectEditorSystem::UpdateEffect(EffectComponent& effect)
{
	effect;
}

void EffectEditorSystem::InitEffectParticle()
{
	// ParticleBuffer
	IRWStructuredBuffer* particleBuffer = m_pResourceManager->GetEffectParticleBuffer();
	// ParticleListBuffer
	IRWStructuredBuffer* particleListBuffer = m_pResourceManager->GetEffectParticleFreeListBuffer();
	// コマンドリスト開始
	CommandContext* context = m_pGraphicsEngine->GetCommandContext();
	m_pGraphicsEngine->BeginCommandContext(context);
	// パイプラインセット
	context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetEffectInitPSO().pso.Get());
	// ルートシグネチャセット
	context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetEffectInitPSO().rootSignature.Get());
	// Particleバッファをセット
	context->SetComputeRootDescriptorTable(0, particleBuffer, ViewType::UnorderedAccessView);
	// ParticleListバッファをセット
	context->SetComputeRootDescriptorTable(1, particleListBuffer, ViewType::UnorderedAccessView);
	// ListCounterバッファをセット
	context->SetComputeRootUnorderedAccessView(2, particleListBuffer);
	// Dispatch
	context->Dispatch(128, 1, 1);
	// 並列阻止
	context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, particleBuffer->GetResource());
	context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, particleListBuffer->GetResource());
	// クローズ
	m_pGraphicsEngine->EndCommandContext(context, QueueType::Compute);
}

void EffectEditorSystem::UpdateShader()
{
	if (!m_pResourceManager->GetEffectRootUseListCount()) { return; }

	// バッファを取得
	IStructuredBuffer* useRootListBuffer = m_pResourceManager->GetEffectRootUseListBuffer();
	IStructuredBuffer* rootBuffer = m_pResourceManager->GetIntegrationBuffer(IntegrationDataType::EffectRootInt);
	IStructuredBuffer* nodeBuffer = m_pResourceManager->GetIntegrationBuffer(IntegrationDataType::EffectNodeInt);
	IStructuredBuffer* spriteBuffer = m_pResourceManager->GetIntegrationBuffer(IntegrationDataType::EffectSpriteInt);
	IRWStructuredBuffer* particleBuffer = m_pResourceManager->GetEffectParticleBuffer();
	IRWStructuredBuffer* particleListBuffer = m_pResourceManager->GetEffectParticleFreeListBuffer();

	// コマンドリスト取得
	CommandContext* context = m_pGraphicsEngine->GetCommandContext();
	// コマンドリスト開始
	m_pGraphicsEngine->BeginCommandContext(context);

	// 発生
	// パイプラインセット
	context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetEffectTimeBaseEmitPSO().pso.Get());
	// ルートシグネチャセット
	context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetEffectTimeBaseEmitPSO().rootSignature.Get());
	// UseListをセット
	context->SetComputeRootDescriptorTable(0, useRootListBuffer, ViewType::ShaderResourceView);
	// Rootをセット
	context->SetComputeRootDescriptorTable(1, rootBuffer, ViewType::ShaderResourceView);
	// ノードバッファをセット
	context->SetComputeRootDescriptorTable(2, nodeBuffer, ViewType::ShaderResourceView);
	// スプライトバッファをセット
	context->SetComputeRootDescriptorTable(3, spriteBuffer, ViewType::ShaderResourceView);
	// Particleバッファをセット
	context->SetComputeRootDescriptorTable(4, particleBuffer, ViewType::UnorderedAccessView);
	// ParticleListバッファをセット
	context->SetComputeRootDescriptorTable(5, particleListBuffer, ViewType::UnorderedAccessView);
	// ListCounterバッファをセット
	context->SetComputeRootUnorderedAccessView(6, particleListBuffer);
	// Dispatch
	context->Dispatch(static_cast<UINT>(m_pResourceManager->GetEffectRootUseListCount()), 1, 1);

	// 並列阻止
	context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, particleBuffer->GetResource());
	context->BarrierUAV(D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, particleListBuffer->GetResource());

	// 更新
	// パイプラインセット
	context->SetComputePipelineState(m_pGraphicsEngine->GetPipelineManager()->GetEffectTimeBaseUpdatePSO().pso.Get());
	// ルートシグネチャセット
	context->SetComputeRootSignature(m_pGraphicsEngine->GetPipelineManager()->GetEffectTimeBaseUpdatePSO().rootSignature.Get());
	// Rootをセット
	context->SetComputeRootDescriptorTable(0, rootBuffer, ViewType::ShaderResourceView);
	// ノードバッファをセット
	context->SetComputeRootDescriptorTable(1, nodeBuffer, ViewType::ShaderResourceView);
	// スプライトバッファをセット
	context->SetComputeRootDescriptorTable(2, spriteBuffer, ViewType::ShaderResourceView);
	// Particleバッファをセット
	context->SetComputeRootDescriptorTable(3, particleBuffer, ViewType::UnorderedAccessView);
	// ParticleListバッファをセット
	context->SetComputeRootDescriptorTable(4, particleListBuffer, ViewType::UnorderedAccessView);
	// Dispatch
	context->Dispatch(128, 1, 1);
	// クローズ
	m_pGraphicsEngine->EndCommandContext(context, QueueType::Compute);
}

void Rigidbody3DSystem::InitializeComponent(Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
{
	e; transform; rb;
}

void Rigidbody3DSystem::UpdateComponent(Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
{
	e;
	if (!rb.runtimeBody) return;

	// サイズが変更されていたらボディ再作成
	if (rb.halfsize != rb.preHalfsize)
	{
		// ボディを削除
		m_World->DestroyBody(rb.runtimeBody);
		rb.runtimeBody = nullptr;
		// 新しいボディを作成
		physics::d3::Id3BodyDef bodyDef;
		bodyDef.position = float3(transform.position.x, transform.position.y, transform.position.z);
		// bodyDef.userData = static_cast<void*>(&rb.selfEntity.value());
		bodyDef.userIndex = static_cast<int>(rb.selfEntity.value());
		bodyDef.friction = rb.friction;
		bodyDef.restitution = rb.restitution;
		bodyDef.mass = rb.mass;
		bodyDef.halfsize = rb.halfsize;
		rb.runtimeBody = m_World->CreateBody(bodyDef);
	}

	rb.runtimeBody->SetBodyType(m_World, rb.bodyType);
	rb.runtimeBody->SetSensor(rb.isSensor);
	rb.runtimeBody->SetGravityScale(m_World,rb.gravityScale);

	if (rb.requestedPosition)
	{
		rb.runtimeBody->SetTransform(rb.requestedPosition.value(), rb.runtimeBody->GetQuaternion());
		rb.requestedPosition.reset();
	}

	// 剛体のTransformをTransformComponentに反映
	// position
	transform.position = rb.runtimeBody->GetPosition();
	// rotation
	transform.degrees = rb.runtimeBody->GetRotation();
	transform.quaternion = rb.runtimeBody->GetQuaternion();
	
	// Rigidbody3DComponentに反映
	// velocity
	float3 velocity = rb.runtimeBody->GetLinearVelocity();
	rb.velocity.x = velocity.x;
	rb.velocity.y = velocity.y;
	rb.velocity.z = velocity.z;
	// angularVelocity
	float3 angularVelocity = rb.runtimeBody->GetAngularVelocity();
	rb.angularVelocity.x = angularVelocity.x;
	rb.angularVelocity.y = angularVelocity.y;
	rb.angularVelocity.z = angularVelocity.z;
	// quaternion
	rb.quaternion = rb.runtimeBody->GetQuaternion();
	// halfsize
	rb.preHalfsize = rb.halfsize;
}

void Rigidbody3DSystem::Reset(Rigidbody3DComponent& rb)
{
	// Bodyがあるなら削除
	if (rb.runtimeBody)
	{
		/*rb.runtimeBody->DestroyShape();
		m_World->DestroyBody(rb.runtimeBody);*/
		m_World->DestroyBody(rb.runtimeBody);
	}
	rb.runtimeBody = nullptr;
	rb.otherEntity.reset();
}

void Rigidbody3DSystem::FinalizeComponent(Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
{
	e; transform;
	Reset(rb);
}

void Rigidbody3DSystem::StepSimulation()
{
	if (m_Paused)
	{
		m_World->Step(0.0f);// ゼロステップで更新しない
		return;
	}
	float deltaTime = DeltaTime();
	if (deltaTime <= 0.0f) return; // 0以下は無視
	m_World->Step(deltaTime);
}

void Rigidbody3DSystem::AwakeComponent(Entity e, TransformComponent& transform, Rigidbody3DComponent& rb)
{
	e;
	physics::d3::Id3BodyDef bodyDef;
	// Rigidbody3DComponentの初期化
	bodyDef.position = transform.position;
	bodyDef.degrees = transform.degrees;
	bodyDef.userIndex = static_cast<int>(rb.selfEntity.value());
	bodyDef.friction = rb.friction;
	bodyDef.restitution = rb.restitution;
	bodyDef.halfsize = rb.halfsize;

	rb.velocity.Initialize();

	rb.runtimeBody = m_World->CreateBody(bodyDef);
	
	rb.runtimeBody->SetBodyType(m_World, rb.bodyType);

	rb.runtimeBody->SetLinearVelocity(rb.velocity);
}
