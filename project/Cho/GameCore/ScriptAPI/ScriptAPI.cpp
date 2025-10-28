#include "pch.h"
#include "ScriptAPI.h"
#include "Platform/InputManager/InputManager.h"
#include "GameCore/ECS/ECSManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Resources/AudioManager/AudioManager.h"

//class Rigidbody2DAPI::ImplRigidbody2DAPI
//{
//public:
//	ImplRigidbody2DAPI() = default;
//	~ImplRigidbody2DAPI() = default;
//	std::function<float2()> GetLastHitNormalFunc;
//	std::function<b2Vec2(const b2Vec2& incident, const b2Vec2& normal)> ReflectFunc;
//	std::function<b2Vec2(const b2Vec2& start, const b2Vec2& dir, const int ReflectionCount, const float maxLength, const std::string hitTag)> RaycastWithReflectionsOnceFunc;
//	std::function<void(const float2& position)> MovePositionFunc;
//	std::function<GameObject* (const float2& start, const float2& end, const std::string hitTag)> LinecastFunc;
//	std::function<void(bool isAwake)> SetAwakeFunc;
//	// 最後の法線（内部的に保持、ただし状態は保持しないなら静的でもよい）
//	b2Vec2 m_LastHitNormal = b2Vec2(0.0f, 1.0f); // 一時的な用途
//};
//Rigidbody2DAPI::Rigidbody2DAPI() : implRigidbody2DAPI(new Rigidbody2DAPI::ImplRigidbody2DAPI) {}
//Rigidbody2DAPI::~Rigidbody2DAPI() { delete implRigidbody2DAPI; }
//
//float2 Rigidbody2DAPI::GetLastHitNormal() const
//{
//	if (implRigidbody2DAPI->GetLastHitNormalFunc) { return implRigidbody2DAPI->GetLastHitNormalFunc(); }
//	return float2();
//}
//
//void Rigidbody2DAPI::Reflect(const b2Vec2& incident, const b2Vec2& normal)
//{
//	if (implRigidbody2DAPI->ReflectFunc) { implRigidbody2DAPI->ReflectFunc(incident, normal); }
//}
//
//b2Vec2 Rigidbody2DAPI::RaycastWithReflectionsOnce(const b2Vec2& start, const b2Vec2& dir, const int ReflectionCount, const float maxLength, const std::string hitTag)
//{
//	return implRigidbody2DAPI->RaycastWithReflectionsOnceFunc(start, dir, ReflectionCount, maxLength, hitTag);
//}
//
//void Rigidbody2DAPI::MovePosition(const float2& position)
//{
//	if (implRigidbody2DAPI->MovePositionFunc) { implRigidbody2DAPI->MovePositionFunc(position); }
//}
//
//GameObject* Rigidbody2DAPI::Linecast(const float2& start, const float2& end, const std::string hitTag)
//{
//	return implRigidbody2DAPI->LinecastFunc(start, end, hitTag);
//}
//
//void Rigidbody2DAPI::SetAwake(bool isAwake)
//{
//	if (implRigidbody2DAPI->SetAwakeFunc) { implRigidbody2DAPI->SetAwakeFunc(isAwake); }
//}
//
//void Rigidbody2DAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// Rigidbody2DComponent を取得
//	Rigidbody2DComponent* rb = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity);
//	data = rb;
//	if (rb)
//	{
//		implRigidbody2DAPI->GetLastHitNormalFunc = [this]() -> float2 {
//			if (auto* t = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity))
//			{
//				return float2(implRigidbody2DAPI->m_LastHitNormal.x, implRigidbody2DAPI->m_LastHitNormal.y);
//			}
//			return float2();
//			};
//		implRigidbody2DAPI->ReflectFunc = [this](const b2Vec2& incident, const b2Vec2& normal) -> b2Vec2 {
//			if (auto* t = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity))
//			{
//				return incident - 2.0f * b2Dot(incident, normal) * normal;
//			}
//			return {};
//			};
//		implRigidbody2DAPI->RaycastWithReflectionsOnceFunc = [this](const b2Vec2& start, const b2Vec2& dir, const int ReflectionCount, const float maxLength, const std::string hitTag) -> b2Vec2 {
//			b2Vec2 resultPoint = {};
//			if (auto* t = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity))
//			{
//				b2Vec2 currentStart = start;
//				b2Vec2 currentDir = dir;
//				float remainingLength = maxLength;
//				for (int i = 0; i < ReflectionCount; ++i)
//				{
//					RayCastCallback callback(m_ObjectContainer, hitTag);
//					b2Vec2 end = currentStart + remainingLength * currentDir;
//					data->world->RayCast(&callback, currentStart, end);
//					if (callback.hit)
//					{
//						resultPoint = callback.point;
//						float hitDistance = remainingLength * callback.fraction;
//						remainingLength -= hitDistance;
//						currentStart = callback.point;
//						currentDir = implRigidbody2DAPI->ReflectFunc(currentDir, callback.normal);
//					} else
//					{
//						currentDir *= remainingLength;
//						resultPoint = currentStart + currentDir;
//						break;
//					}
//				}
//			}
//			return resultPoint;
//			};
//		implRigidbody2DAPI->MovePositionFunc = [this](const float2& position) {
//			if (auto* t = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity))
//			{
//				if (t->runtimeBody)
//				{
//					t->requestedPosition = b2Vec2(position.x, position.y);
//				}
//			}
//			};
//		implRigidbody2DAPI->LinecastFunc = [this](const float2& start, const float2& end, const std::string hitTag) -> GameObject* {
//			if (auto* t = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity))
//			{
//				RayCastCallback callback(m_ObjectContainer, hitTag);
//				if(start == end)
//				{
//					// start と end が同じ場合は何もしない
//					return nullptr;
//				}
//				t->world->RayCast(&callback, b2Vec2(start.x, start.y), b2Vec2(end.x, end.y));
//				if (callback.hit)
//				{
//					Entity entity = static_cast<Entity>(callback.fixture->GetBody()->GetUserData().pointer);
//					return m_ObjectContainer->GetGameObject(entity);
//				}
//			}
//			return nullptr;
//			};
//		implRigidbody2DAPI->SetAwakeFunc = [this](bool isAwake) {
//			if (auto* t = m_ECS->GetComponent<Rigidbody2DComponent>(m_Entity))
//			{
//				if (t->runtimeBody)
//				{
//					t->runtimeBody->SetAwake(isAwake);
//				}
//			}
//			};
//	}
//}
//
//class InputAPI::ImplInputAPI
//{
//public:
//	ImplInputAPI() = default;
//	~ImplInputAPI() = default;
//	std::function<bool(const uint8_t& keyNumber)> PushKeyFunc;
//	std::function<bool(const uint8_t& keyNumber)> TriggerKeyFunc;
//	std::function<const DIMOUSESTATE2& ()> GetAllMouseFunc;
//	std::function<MouseMove()> GetMouseMoveFunc;
//	std::function<bool(const int32_t& mouseNumber)> IsPressMouseFunc;
//	std::function<bool(const int32_t& buttonNumber)> IsTriggerMouseFunc;
//	std::function<const float2& ()> GetMouseWindowPositionFunc;
//	std::function<float2()> GetMouseScreenPositionFunc;
//	std::function<bool(const int32_t& stickNo, XINPUT_STATE& out)> GetJoystickStateFunc;
//	std::function<bool(const int32_t& stickNo, XINPUT_STATE& out)> GetJoystickStatePreviousFunc;
//	std::function<void(const int32_t& stickNo, const int32_t& deadZoneL, const int32_t& deadZoneR)> SetJoystickDeadZoneFunc;
//	std::function<size_t()> GetNumberOfJoysticksFunc;
//	std::function<bool(const PadButton& button, int32_t stickNo)> IsTriggerPadButtonFunc;
//	std::function<bool(const PadButton& button, int32_t stickNo)> IsPressPadButtonFunc;
//	std::function<float2(const LR& padStick, int32_t stickNo)> GetStickValueFunc;
//	std::function<float(const LR& LorR, int32_t stickNo)> GetLRTriggerFunc;
//};
//InputAPI::InputAPI() : implInputAPI(new InputAPI::ImplInputAPI) {}
//InputAPI::~InputAPI() { delete implInputAPI; }
//
//bool InputAPI::PushKey(const uint8_t& keyNumber)
//{
//	return implInputAPI->PushKeyFunc(keyNumber);
//}
//
//bool InputAPI::TriggerKey(const uint8_t& keyNumber)
//{
//	return implInputAPI->TriggerKeyFunc(keyNumber);
//}
//
//const DIMOUSESTATE2& InputAPI::GetAllMouse()
//{
//	return implInputAPI->GetAllMouseFunc();
//}
//
//MouseMove InputAPI::GetMouseMove()
//{
//	return implInputAPI->GetMouseMoveFunc();
//}
//
//bool InputAPI::IsPressMouse(const int32_t& mouseNumber)
//{
//	return implInputAPI->IsPressMouseFunc(mouseNumber);
//}
//
//bool InputAPI::IsTriggerMouse(const int32_t& buttonNumber)
//{
//	return implInputAPI->IsTriggerMouseFunc(buttonNumber);
//}
//
//const float2& InputAPI::GetMouseWindowPosition()
//{
//	return implInputAPI->GetMouseWindowPositionFunc();
//}
//
//float2 InputAPI::GetMouseScreenPosition()
//{
//	return implInputAPI->GetMouseScreenPositionFunc();
//}
//
//bool InputAPI::GetJoystickState(const int32_t& stickNo, XINPUT_STATE& out)
//{
//	return implInputAPI->GetJoystickStateFunc(stickNo, out);
//}
//
//bool InputAPI::GetJoystickStatePrevious(const int32_t& stickNo, XINPUT_STATE& out)
//{
//	return implInputAPI->GetJoystickStatePreviousFunc(stickNo, out);
//}
//
//void InputAPI::SetJoystickDeadZone(const int32_t& stickNo, const int32_t& deadZoneL, const int32_t& deadZoneR)
//{
//	if (implInputAPI->SetJoystickDeadZoneFunc) { implInputAPI->SetJoystickDeadZoneFunc(stickNo, deadZoneL, deadZoneR); }
//}
//
//size_t InputAPI::GetNumberOfJoysticks()
//{
//	return implInputAPI->GetNumberOfJoysticksFunc();
//}
//
//bool InputAPI::IsTriggerPadButton(const PadButton& button, int32_t stickNo)
//{
//	return implInputAPI->IsTriggerPadButtonFunc(button, stickNo);
//}
//
//bool InputAPI::IsPressPadButton(const PadButton& button, int32_t stickNo)
//{
//	return implInputAPI->IsPressPadButtonFunc(button, stickNo);
//}
//
//float2 InputAPI::GetStickValue(const LR& padStick, int32_t stickNo)
//{
//	return implInputAPI->GetStickValueFunc(padStick, stickNo);
//}
//
//float InputAPI::GetLRTrigger(const LR& LorR, int32_t stickNo)
//{
//	return implInputAPI->GetLRTriggerFunc(LorR, stickNo);
//}
//
//void InputAPI::Intialize(InputManager* input)
//{
//	data = input;
//	implInputAPI->PushKeyFunc = [this](const uint8_t& keyNumber) -> bool {
//		return data->PushKey(keyNumber);
//		};
//	implInputAPI->TriggerKeyFunc = [this](const uint8_t& keyNumber) -> bool {
//		return data->TriggerKey(keyNumber);
//		};
//	implInputAPI->GetAllMouseFunc = [this]() -> const DIMOUSESTATE2& {
//		return data->GetAllMouse();
//		};
//	implInputAPI->GetMouseMoveFunc = [this]() -> MouseMove {
//		return data->GetMouseMove();
//		};
//	implInputAPI->IsPressMouseFunc = [this](const int32_t& mouseNumber) -> bool {
//		return data->IsPressMouse(mouseNumber);
//		};
//	implInputAPI->IsTriggerMouseFunc = [this](const int32_t& buttonNumber) -> bool {
//		return data->IsTriggerMouse(buttonNumber);
//		};
//	implInputAPI->GetMouseWindowPositionFunc = [this]() -> const float2& {
//		return data->GetMouseWindowPosition();
//		};
//	implInputAPI->GetMouseScreenPositionFunc = [this]() -> float2 {
//		return data->GetMouseScreenPosition();
//		};
//	implInputAPI->GetJoystickStateFunc = [this](const int32_t& stickNo, XINPUT_STATE& out) -> bool {
//		return data->GetJoystickState(stickNo, out);
//		};
//	implInputAPI->GetJoystickStatePreviousFunc = [this](const int32_t& stickNo, XINPUT_STATE& out) -> bool {
//		return data->GetJoystickStatePrevious(stickNo, out);
//		};
//	implInputAPI->SetJoystickDeadZoneFunc = [this](const int32_t& stickNo, const int32_t& deadZoneL, const int32_t& deadZoneR) {
//		data->SetJoystickDeadZone(stickNo, deadZoneL, deadZoneR);
//		};
//	implInputAPI->GetNumberOfJoysticksFunc = [this]() -> size_t {
//		return data->GetNumberOfJoysticks();
//		};
//	implInputAPI->IsTriggerPadButtonFunc = [this](const PadButton& button, int32_t stickNo) -> bool {
//		return data->IsTriggerPadButton(button, stickNo);
//		};
//	implInputAPI->IsPressPadButtonFunc = [this](const PadButton& button, int32_t stickNo) -> bool {
//		return data->IsPressPadButton(button, stickNo);
//		};
//	implInputAPI->GetStickValueFunc = [this](const LR& padStick, int32_t stickNo) -> float2 {
//		return data->GetStickValue(padStick, stickNo);
//		};
//	implInputAPI->GetLRTriggerFunc = [this](const LR& LorR, int32_t stickNo) -> float {
//		return data->GetLRTrigger(LorR, stickNo);
//		};
//}
//
//class EmitterAPI::ImplEmitterAPI
//{
//public:
//	ImplEmitterAPI() = default;
//	~ImplEmitterAPI() = default;
//	std::function<void(const float3& position)> SetPositionFunc;
//	std::function<void(const float3& velocity)> SetVelocityFunc;
//	std::function<void(const float3& acceleration)> SetAccelerationFunc;
//	std::function<void(const float3& rotation)> SetRotationFunc;
//	std::function<void(const float3& scale)> SetScaleFunc;
//	std::function<void(const float3& color)> SetColorFunc;
//};
//EmitterAPI::EmitterAPI() : implEmitterAPI(new EmitterAPI::ImplEmitterAPI) {}
//EmitterAPI::~EmitterAPI() { delete implEmitterAPI; }
//
//void EmitterAPI::SetPosition(const float3& position)
//{
//	if (implEmitterAPI->SetPositionFunc) { implEmitterAPI->SetPositionFunc(position); }
//}
//
//void EmitterAPI::SetVelocity(const float3& velocity)
//{
//	if (implEmitterAPI->SetVelocityFunc) { implEmitterAPI->SetVelocityFunc(velocity); }
//}
//
//void EmitterAPI::SetAcceleration(const float3& acceleration)
//{
//	if (implEmitterAPI->SetAccelerationFunc) { implEmitterAPI->SetAccelerationFunc(acceleration); }
//}
//
//void EmitterAPI::SetRotation(const float3& rotation)
//{
//	if (implEmitterAPI->SetRotationFunc) { implEmitterAPI->SetRotationFunc(rotation); }
//}
//
//void EmitterAPI::SetScale(const float3& scale)
//{
//	if (implEmitterAPI->SetScaleFunc) { implEmitterAPI->SetScaleFunc(scale); }
//}
//
//void EmitterAPI::SetColor(const float3& color)
//{
//	if (implEmitterAPI->SetColorFunc) { implEmitterAPI->SetColorFunc(color); }
//}
//
//void EmitterAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// EmitterComponent を取得
//	EmitterComponent* emitter = m_ECS->GetComponent<EmitterComponent>(m_Entity);
//	data = emitter;
//	if (emitter)
//	{
//		/*implEmitterAPI->SetPositionFunc = [this](const float3& position) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				t->position = position;
//			}
//			};
//		implEmitterAPI->SetVelocityFunc = [this](const float3& velocity) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				t->velocity = velocity;
//			}
//			};
//		implEmitterAPI->SetAccelerationFunc = [this](const float3& acceleration) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				t->acceleration = acceleration;
//			}
//			};
//		implEmitterAPI->SetRotationFunc = [this](const float3& rotation) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				t->rotation = rotation;
//			}
//			};
//		implEmitterAPI->SetScaleFunc = [this](const float3& scale) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				t->scale = scale;
//			}
//			};
//		implEmitterAPI->SetColorFunc = [this](const float3& color) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				t->color = color;
//			}
//			};*/
//	}
//}
//
//class ParticleAPI::ImplParticleAPI
//{
//public:
//	ImplParticleAPI() = default;
//	~ImplParticleAPI() = default;
//	std::function<void(const float3& position)> EmitFunc;
//};
//ParticleAPI::ParticleAPI() : implParticleAPI(new ParticleAPI::ImplParticleAPI) {}
//ParticleAPI::~ParticleAPI() { delete implParticleAPI; }
//
//void ParticleAPI::Emit(const float3& position)
//{
//	if (implParticleAPI->EmitFunc) { implParticleAPI->EmitFunc(position); }
//}
//
//void ParticleAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// EmitterComponent を取得
//	EmitterComponent* particle = m_ECS->GetComponent<EmitterComponent>(m_Entity);
//	data = particle;
//	if (particle)
//	{
//		implParticleAPI->EmitFunc = [this](const float3& position) {
//			if (auto* t = m_ECS->GetComponent<EmitterComponent>(m_Entity))
//			{
//				TransformComponent* transform = m_ECS->GetComponent<TransformComponent>(m_Entity);
//				if (transform)
//				{
//					transform->position.x = position.x;
//					data->emit = true; // パーティクルを発生させる
//				}
//			}
//			};
//	}
//}
//
//class EffectAPI::ImplEffectAPI
//{
//public:
//	ImplEffectAPI() = default;
//	~ImplEffectAPI() = default;
//	std::function<void(const float3& position)> SetPositionFunc;
//	std::function<void(const float3& rotation)> SetRotationFunc;
//	std::function<void(const float3& scale)> SetScaleFunc;
//	std::function<void(const float3& color)> SetColorFunc;
//};
//EffectAPI::EffectAPI() : implEffectAPI(new EffectAPI::ImplEffectAPI) {}
//EffectAPI::~EffectAPI() { delete implEffectAPI; }
//
//void EffectAPI::SetPosition(const float3& position)
//{
//	if (implEffectAPI->SetPositionFunc) { implEffectAPI->SetPositionFunc(position); }
//}
//
//void EffectAPI::SetRotation(const float3& rotation)
//{
//	if (implEffectAPI->SetRotationFunc) { implEffectAPI->SetRotationFunc(rotation); }
//}
//
//void EffectAPI::SetScale(const float3& scale)
//{
//	if (implEffectAPI->SetScaleFunc) { implEffectAPI->SetScaleFunc(scale); }
//}
//
//void EffectAPI::SetColor(const float3& color)
//{
//	if (implEffectAPI->SetColorFunc) { implEffectAPI->SetColorFunc(color); }
//}
//
//void EffectAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// EffectComponent を取得
//	EffectComponent* effect = m_ECS->GetComponent<EffectComponent>(m_Entity);
//	data = effect;
//	if (effect)
//	{
//		/*implEffectAPI->SetPositionFunc = [this](const float3& position) {
//			if (auto* t = m_ECS->GetComponent<EffectComponent>(m_Entity))
//			{
//				t->position = position;
//			}
//			};*/
//	}
//}
//
//class EditorAPI::ImplEditorAPI
//{
//public:
//	ImplEditorAPI() = default;
//	~ImplEditorAPI() = default;
//	std::function<void(const char* name)> BeginFunc;
//	std::function<void()> EndFunc;
//	std::function<void(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format)> DragFloatFunc;
//	std::function<void(const char* label, float* v[2], float v_speed, float v_min, float v_max, const char* format)> DragFloat2Func;
//	std::function<void(const char* label, float* v[3], float v_speed, float v_min, float v_max, const char* format)> DragFloat3Func;
//};
//EditorAPI::EditorAPI() : implEditorAPI(new EditorAPI::ImplEditorAPI) {}
//EditorAPI::~EditorAPI() { delete implEditorAPI; }
//void EditorAPI::Begin(const char* name)
//{
//	if (implEditorAPI->BeginFunc) { implEditorAPI->BeginFunc(name); }
//}
//
//void EditorAPI::End()
//{
//	if (implEditorAPI->EndFunc) { implEditorAPI->EndFunc(); }
//}
//
//void EditorAPI::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format)
//{
//	if (implEditorAPI->DragFloatFunc) { implEditorAPI->DragFloatFunc(label, v, v_speed, v_min, v_max, format); }
//}
//
//void EditorAPI::DragFloat2(const char* label, float* v[2], float v_speed, float v_min, float v_max, const char* format)
//{
//	if (implEditorAPI->DragFloat2Func) { implEditorAPI->DragFloat2Func(label, v, v_speed, v_min, v_max, format); }
//}
//
//void EditorAPI::DragFloat3(const char* label, float* v[3], float v_speed, float v_min, float v_max, const char* format)
//{
//	if (implEditorAPI->DragFloat3Func) { implEditorAPI->DragFloat3Func(label, v, v_speed, v_min, v_max, format); }
//}
//
//void EditorAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	//if (!m_ECS || !m_ResourceManager)
//	//{
//	//	data = nullptr;
//	//	return;
//	//}
//	//// EditorComponent を取得
//	//EditorComponent* editor = m_ECS->GetComponent<EditorComponent>(m_Entity);
//	//data = editor;
//	//if (editor)
//	//{
//	//	implEditorAPI->BeginFunc = [this](const char* name) {
//	//		if (auto* t = m_ECS->GetComponent<EditorComponent>(m_Entity))
//	//		{
//	//			t->Begin(name);
//	//		}
//	//		};
//	//	implEditorAPI->EndFunc = [this]() {
//	//		if (auto* t = m_ECS->GetComponent<EditorComponent>(m_Entity))
//	//		{
//	//			t->End();
//	//		}
//	//		};
//	//}
//}
//
//class AudioAPI::ImplAudioAPI
//{
//public:
//	ImplAudioAPI() = default;
//	~ImplAudioAPI() = default;
//	std::function<void(const std::string&)> SetSourceFunc;
//	std::function<void(const std::string&, const bool&)> PlayFunc;
//	std::function<void(const std::string&)> StopFunc;
//	std::function<bool(const std::string&)> IsPlayingFunc;
//	std::function<void(const std::string&, const float&)> SetVolumeFunc;
//};
//AudioAPI::AudioAPI() : implAudioAPI(new AudioAPI::ImplAudioAPI) {}
//AudioAPI::~AudioAPI() { delete implAudioAPI; }
//void AudioAPI::AddSource(const std::string& name)
//{
//	if(implAudioAPI->SetSourceFunc) { implAudioAPI->SetSourceFunc(name); }
//}
//void AudioAPI::Play(const std::string& name, const bool& isLoop)
//{
//	if (implAudioAPI->PlayFunc) { implAudioAPI->PlayFunc(name,isLoop); }
//}
//
//void AudioAPI::Stop(const std::string& name)
//{
//	if (implAudioAPI->StopFunc) { implAudioAPI->StopFunc(name); }
//}
//
//bool AudioAPI::IsPlaying(const std::string& name)
//{
//	if (implAudioAPI->IsPlayingFunc) { return implAudioAPI->IsPlayingFunc(name); }
//	return false;
//}
//
//void AudioAPI::SetVolume(const std::string& name, const float& volume)
//{
//	if (implAudioAPI->SetVolumeFunc) { implAudioAPI->SetVolumeFunc(name, volume); }
//}
//
////void AudioAPI::SetSource(const std::string& sourceName)
////{
////	if (implAudioAPI->SetSourceFunc) { implAudioAPI->SetSourceFunc(sourceName); }
////}
//
//void AudioAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// AudioComponent を取得
//	AudioComponent* audio = m_ECS->GetComponent<AudioComponent>(m_Entity);
//	data = audio;
//	if (audio)
//	{
//		implAudioAPI->SetSourceFunc = [this](const std::string& name) {
//			if (auto* a = m_ECS->GetComponent<AudioComponent>(m_Entity))
//			{
//				for (SoundData& soundData : data->soundData)
//				{
//					if (soundData.name == name && !soundData.isPlaying)
//					{
//						return;
//					}
//				}
//				// もし指定された名前のサウンドが見つからなかった場合
//				SoundData newSoundData = m_ResourceManager->GetAudioManager()->CreateSoundData(name);
//				if (!newSoundData.name.empty())
//				{
//					data->soundData.push_back(std::move(newSoundData));
//				}
//				return;
//			}
//			};
//		implAudioAPI->PlayFunc = [this](const std::string& name, const bool& isLoop) {
//			if (auto* a = m_ECS->GetComponent<AudioComponent>(m_Entity))
//			{
//				/*m_ResourceManager->GetAudioManager()->SoundPlayWave(a->audioID.value(), a->isLoop);
//				a->isPlay = true;*/
//				for (SoundData& soundData : data->soundData)
//				{
//					if (soundData.name == name && !soundData.isPlaying)
//					{
//						m_ResourceManager->GetAudioManager()->SoundPlayWave(soundData,isLoop);
//						return;
//					}
//				}
//				// もし指定された名前のサウンドが見つからなかった場合
//				SoundData newSoundData = m_ResourceManager->GetAudioManager()->CreateSoundData(name);
//				if (!newSoundData.name.empty())
//				{
//					m_ResourceManager->GetAudioManager()->SoundPlayWave(newSoundData,isLoop);
//					data->soundData.push_back(std::move(newSoundData));
//				}
//				return;
//			}
//			};
//		implAudioAPI->StopFunc = [this](const std::string& name) {
//			if (auto* a = m_ECS->GetComponent<AudioComponent>(m_Entity))
//			{
//				/*m_ResourceManager->GetAudioManager()->SoundStop(a->audioID.value());
//				a->isPlay = false;*/
//				for (SoundData& soundData : data->soundData)
//				{
//					if (soundData.name == name && soundData.isPlaying)
//					{
//						m_ResourceManager->GetAudioManager()->SoundStop(soundData);
//						return;
//					}
//				}
//			}
//			};
//		implAudioAPI->IsPlayingFunc = [this](const std::string& name) -> bool {
//			if (auto* a = m_ECS->GetComponent<AudioComponent>(m_Entity))
//			{
//				for (const SoundData& soundData : data->soundData)
//				{
//					if (soundData.name == name)
//					{
//						return soundData.isPlaying;
//					}
//				}
//			}
//			return false;
//			};
//		implAudioAPI->SetVolumeFunc = [this](const std::string& name, const float& volume) {
//			if (auto* a = m_ECS->GetComponent<AudioComponent>(m_Entity))
//			{
//				for (SoundData& soundData : data->soundData)
//				{
//					if (soundData.name == name)
//					{
//						soundData.currentVolume = volume;
//						m_ResourceManager->GetAudioManager()->SetVolume(soundData, volume);
//						return;
//					}
//				}
//			}
//			};
//		//implAudioAPI->SetSourceFunc = [this](const std::string& sourceName) {
//		//	if (auto* a = m_ECS->GetComponent<AudioComponent>(m_Entity))
//		//	{
//		//		if (a->audioID.has_value()&&a->isPlay)
//		//		{
//		//			m_ResourceManager->GetAudioManager()->SoundStop(a->audioID.value());
//		//		}
//		//		a->audioID = m_ResourceManager->GetAudioManager()->GetSoundDataIndex(sourceName);
//		//		a->isPlay = false; // ソースを変更したら再生状態をリセット
//		//	}
//		//	};
//	}
//}
//
//void TransformAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager, bool isParentReset)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// TransformComponent を取得
//	TransformComponent* transform = m_ECS->GetComponent<TransformComponent>(m_Entity);
//	data = transform;
//	if (isParentReset)
//	{
//		transform->parent.reset();
//	}
//}
//
//void CameraAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// CameraComponent を取得
//	CameraComponent* camera = m_ECS->GetComponent<CameraComponent>(m_Entity);
//	data = camera;
//}
//
//void LineRendererAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// LineRendererComponent を取得
//	auto* line = m_ECS->GetAllComponents<LineRendererComponent>(m_Entity);
//	data = line;
//}
//
//void BoxCollider2DAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// BoxCollider2DComponent を取得
//	BoxCollider2DComponent* box = m_ECS->GetComponent<BoxCollider2DComponent>(m_Entity);
//	data = box;
//}
//
//void MaterialAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// MaterialComponent を取得
//	auto* material = m_ECS->GetComponent<MaterialComponent>(m_Entity);
//	data = material;
//}
//
//void UISpriteAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// UISpriteComponent を取得
//	auto* ui = m_ECS->GetComponent<UISpriteComponent>(m_Entity);
//	data = ui;
//}
//
//class AnimationAPI::ImplAnimationAPI
//{
//	public:
//	ImplAnimationAPI() = default;
//	~ImplAnimationAPI() = default;
//};
//AnimationAPI::AnimationAPI() : implAnimationAPI(new AnimationAPI::ImplAnimationAPI) {}
//AnimationAPI::~AnimationAPI() { delete implAnimationAPI; }
//void AnimationAPI::Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager)
//{
//	m_ECS = ecs;
//	m_ObjectContainer = objectContainer;
//	m_ResourceManager = resourceManager;
//	m_Entity = entity;
//	if (!m_ECS || !m_ResourceManager)
//	{
//		data = nullptr;
//		return;
//	}
//	// AnimationComponent を取得
//	auto* animation = m_ECS->GetComponent<AnimationComponent>(m_Entity);
//	data = animation;
//}

void theatriaSystem::Rigidbody3D::SetTransform(const float3& pos)
{
	data->runtimeBody->SetTransform(pos);
}

void theatriaSystem::Rigidbody3D::SetTransform(const Quaternion& rot)
{
	data->runtimeBody->SetTransform(rot);
}
