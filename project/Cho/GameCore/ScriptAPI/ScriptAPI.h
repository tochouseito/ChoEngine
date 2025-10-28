#pragma once
#include "Core/Utility/Components.h"
#include "Core/Utility/InputStruct.h"
#include "APIExportsMacro.h"

#define REGISTER_SCRIPT_FACTORY(SCRIPTNAME) \
    extern "C" __declspec(dllexport) Marionnette* Create##SCRIPTNAME##Script(GameObject& object) { \
        return new SCRIPTNAME(object);}
//// メンバ登録マクロ
//#define REFLECT_SCRIPT_MEMBER(CLASS, MEMBER) \
//    { #MEMBER, \
//      typeid(decltype(CLASS::MEMBER)), \
//      [](void* obj) -> void* { return &static_cast<CLASS*>(obj)->MEMBER; }, \
//      [](void* obj, void* value) { static_cast<CLASS*>(obj)->MEMBER = *static_cast<decltype(CLASS::MEMBER)*>(value); }}
//// 関数登録マクロ
//#define REFLECT_SCRIPT_FUNC(CLASS, FUNC) \
//    { #FUNC, [](void* obj) { static_cast<CLASS*>(obj)->FUNC(); } }
//#define REGISTER_SCRIPT_REFLECTION(SCRIPTNAME) \
//    extern "C" __declspec(dllexport) IScript* Create##SCRIPTNAME##Script(GameObject& object) { \
//        return new SCRIPTNAME(object); \
//    } \
//    extern "C" __declspec(dllexport) const ScriptTypeInfo* GetScriptTypeInfo##SCRIPTNAME() { \
//        static ScriptTypeInfo info = { \
//            #SCRIPTNAME, \
//            SCRIPTNAME::GetReflectedMembers() \
//        }; \
//        return &info; \
//    } \
//    extern "C" __declspec(dllexport) const std::vector<ScriptFunction>* GetScriptFunctions##SCRIPTNAME() { \
//        return &SCRIPTNAME::GetReflectedFunctions(); \
//    }
//#define BEGIN_SCRIPT_REFLECTION(CLASS) \
//    static void RegisterMembers(std::vector<ScriptMember>& out) {
//
//#define ADD_MEMBER(CLASS, MEMBER) \
//    out.emplace_back(REFLECT_SCRIPT_MEMBER(CLASS, MEMBER));
//
//#define END_SCRIPT_REFLECTION_MEMBERS }
//
//#define BEGIN_SCRIPT_FUNCTIONS(CLASS) \
//    static void RegisterFunctions(std::vector<ScriptFunction>& out) {
//
//#define ADD_FUNCTION(FUNC) \
//    out.emplace_back(REFLECT_SCRIPT_FUNC(CLASS, FUNC));
//
//#define END_SCRIPT_REFLECTION_FUNCTIONS }
//
//#define AUTO_REFLECT(CLASS) \
//    static std::vector<ScriptMember> GetReflectedMembers() { return IScript::BuildReflectedMembers<CLASS>(); } \
//    static std::vector<ScriptFunction> GetReflectedFunctions() { return IScript::BuildReflectedFunctions<CLASS>(); }
//
//struct ScriptMember
//{
//	const char* name;
//	std::type_index type;
//	std::function<void* (void*)> getter;
//	std::function<void(void*, void*)> setter;
//};
//
//struct ScriptFunction
//{
//	const char* name;
//	std::function<void(void*)> invoker;
//};
//struct ScriptTypeInfo
//{
//	const char* className;
//	std::vector<ScriptMember> members;
//};
//using GetTypeInfoFn = const ScriptTypeInfo* (*)();
//using GetFuncListFn = const std::vector<ScriptFunction>* (*)();

class ECSManager;
class ResourceManager;

namespace theatriaSystem
{
    // スクリプトから取得可能なコンポーネントのインターフェース
    // 基底クラス
    class THEATRIA_API IComponentInterface
    {
    public:
        IComponentInterface(Entity e, ECSManager* ecs) :m_Entity(e), m_ECS(ecs) {}
        virtual ~IComponentInterface() = default;
    protected:
        Entity m_Entity; // Entity
        ECSManager* m_ECS = nullptr; // ECSManager
    };

    // Marionnette用特殊
    template<typename T>
    concept MarionnetteInterface = std::derived_from<T, Marionnette>;

    // インターフェース型の許可
    template<typename T>
    concept Type = std::derived_from<T, IComponentInterface>;

    // Traits pattern
    template<typename Interface>
    struct InterfaceTraits;

	// Transform
    class THEATRIA_API Transform : public IComponentInterface
    {
		friend class Marionnette;
    public:
        Transform(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Transform() = default;
        TransformComponent* operator->()
        {
            UpdatePtr();
            return data;
		}
        operator bool() noexcept
        {
            UpdatePtr();
			return data != nullptr;
        }
    private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<TransformComponent>(m_Entity);
        }
        TransformComponent* data = nullptr;
    public:
	};
	template<>
    struct InterfaceTraits<theatriaSystem::Transform>
    {
        using Component = TransformComponent;
	};

    // Script
    class THEATRIA_API Script : public IComponentInterface
    {
        friend class Marionnette;
    public:
        Script(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Script() = default;
        ScriptComponent* operator->()
        {
            UpdatePtr();
            return data;
        }
        operator bool() noexcept
        {
            UpdatePtr();
            return data != nullptr;
        }
    private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<ScriptComponent>(m_Entity);
        }
        ScriptComponent* data = nullptr;
    };

    // Camera
    class THEATRIA_API Camera : public IComponentInterface
    {
        friend class Marionnette;
    public:
        Camera(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Camera() = default;
        CameraComponent* operator->()
        {
            UpdatePtr();
            return data;
        }
        operator bool() noexcept
        {
            UpdatePtr();
            return data != nullptr;
        }
    private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<CameraComponent>(m_Entity);
        }
        CameraComponent* data = nullptr;
    };
    template<>
    struct InterfaceTraits<theatriaSystem::Camera>
    {
        using Component = CameraComponent;
    };


    // Material
    class THEATRIA_API Material : public IComponentInterface
    {
        friend class Marionnette;
    public:
        Material(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Material() = default;
        MaterialComponent* operator->()
        {
            UpdatePtr();
            return data;
        }
        operator bool() noexcept
        {
            UpdatePtr();
            return data != nullptr;
        }
    private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<MaterialComponent>(m_Entity);
        }
        MaterialComponent* data = nullptr;
    };
    template<>
    struct InterfaceTraits<theatriaSystem::Material>
    {
        using Component = MaterialComponent;
    };

    // Rigidbody3D
    class THEATRIA_API Rigidbody3D : public IComponentInterface
    {
		friend class Marionnette;
    public:
        Rigidbody3D(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Rigidbody3D() = default;
        Rigidbody3DComponent* operator->()
        {
            UpdatePtr();
            return data;
		}
        operator bool() noexcept
        {
            UpdatePtr();
            return data != nullptr;
        }

        void SetTransform(const float3& pos);
        void SetTransform(const Quaternion& rot);

    private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<Rigidbody3DComponent>(m_Entity);
        }
		Rigidbody3DComponent* data = nullptr;
	};

    // Audio
    class THEATRIA_API Audio : public IComponentInterface
    {
		friend class Marionnette;
        public:
        Audio(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Audio() = default;
        AudioComponent* operator->()
        {
            UpdatePtr();
			return data;
            }
        operator bool() noexcept
        {
			UpdatePtr();
            return data != nullptr;
		}

        /*void AddSource(const std::string& name)
        {

        }
		void Play(const std::string& name, const bool& isLoop);
		void Stop(const std::string& name);
		bool IsPlaying(const std::string& name);
		void SetVolume(const std::string& name, const float& volume);*/
        private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<AudioComponent>(m_Entity);
		}
        AudioComponent* data = nullptr;
	};

    // Animation
    class THEATRIA_API Animation : public IComponentInterface
    {
        friend class Marionnette;
    public:
        Animation(Entity e, ECSManager* ecs) : IComponentInterface(e, ecs) {}
        ~Animation() = default;
        AnimationComponent* operator->()
        {
            UpdatePtr();
            return data;
        }
        operator bool() noexcept
        {
            UpdatePtr();
            return data != nullptr;
        }
    private:
        void UpdatePtr()
        {
            data = m_ECS->GetComponent<AnimationComponent>(m_Entity);
        }
        AnimationComponent* data = nullptr;
    };
    template<>
    struct InterfaceTraits<theatriaSystem::Animation>
    {
        using Component = AnimationComponent;
    };
}

//struct CHO_API TransformAPI
//{
//	// 関数
//	float3& position() { return data->position; }
//	float3& rotation() { return data->degrees; }
//	Quaternion& quaternion() { return data->rotation; }
//	Scale& scale() { return data->scale; }
//	float4x4& worldMatrix() { return data->matWorld; }
//	void SetParent(Entity parent) { data->parent = parent; }
//	float3& forward() { return data->forward; }
//
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager, bool isParentReset = false);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	TransformComponent* data = nullptr;
//};
//
//struct CHO_API CameraAPI
//{
//	// 関数
//	float& fovAngleY() { return data->fovAngleY; }
//	float& aspectRatio() { return data->aspectRatio; }
//	float& nearZ() { return data->nearZ; }
//	float& farZ() { return data->farZ; }
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	CameraComponent* data = nullptr;
//};
//
//struct CHO_API LineRendererAPI
//{
//	// 関数
//	float3& start(uint32_t index) { return (*data)[index].line.start; }
//	float3& end(uint32_t index) { return (*data)[index].line.end; }
//	Color& color(uint32_t index) { return (*data)[index].line.color; }
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	std::vector<LineRendererComponent>* data = nullptr;
//};
//
//struct CHO_API Rigidbody2DAPI
//{
//public:
//	Rigidbody2DAPI();
//	~Rigidbody2DAPI();
//	// 関数
//	float2& velocity() { return data->velocity; }
//	void SetBodyType(b2BodyType type) { data->bodyType = type; }
//	void SetFixedRotation(bool fixed) { data->fixedRotation = fixed; }
//	void SetActive(bool active) { data->isActive = active; }
//
//	// 関数ポインタのラッパー
//	// 法線取得（RaycastOnce の直後のみ有効）
//	float2 GetLastHitNormal() const;
//	// 反射ベクトルを計算
//	void Reflect(const b2Vec2& incident, const b2Vec2& normal);
//	// 反射Raycast（指定回数分反射する）
//	b2Vec2 RaycastWithReflectionsOnce(const b2Vec2& start, const b2Vec2& dir, const int ReflectionCount, const float maxLength, const std::string hitTag);
//	// 瞬間移動
//	void MovePosition(const float2& position);
//	// ライン上の最初にヒットしたオブジェクトを取得
//	GameObject* Linecast(const float2& start, const float2& end, const std::string hitTag);
//	// 強制的に物理計算
//	void SetAwake(bool isAwake);
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity,ECSManager* ecs,ObjectContainer* objectContainer,ResourceManager* resourceManager);
//	// 実装隠蔽クラス
//	class ImplRigidbody2DAPI;
//	ImplRigidbody2DAPI* implRigidbody2DAPI = nullptr;
//	friend class GameObject;
//	Rigidbody2DComponent* data = nullptr;
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//};
//
//// BoxCollider2DAPI
//struct CHO_API BoxCollider2DAPI
//{
//	// 関数
//	float& offsetX() { return data->offsetX; }
//	float& offsetY() { return data->offsetY; }
//	float& width() { return data->width; }
//	float& height() { return data->height; }
//	bool IsSensor() const { return data->isSensor; }
//	void SetSensor(bool isSensor) { data->isSensor = isSensor; }
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	BoxCollider2DComponent* data = nullptr;
//};
//
//// MaterialAPI
//struct CHO_API MaterialAPI
//{
//	// 関数
//	Color& color() { return data->color; }
//	std::wstring& textureName() { return data->textureName; }
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	MaterialComponent* data = nullptr;
//};
//
////InputAPI
//class InputManager;
//struct CHO_API InputAPI
//{
//public:
//	InputAPI();
//	~InputAPI();
//	// 関数ポインタのラッパー
//	// キーの押下をチェック
//	bool PushKey(const uint8_t& keyNumber);
//	// キーのトリガーをチェック
//	bool TriggerKey(const uint8_t& keyNumber);
//	// 全マウス情報取得
//	const DIMOUSESTATE2& GetAllMouse();
//	// マウス移動量を取得
//	MouseMove GetMouseMove();
//	// マウスの押下をチェック
//	bool IsPressMouse(const int32_t& mouseNumber);
//	// マウスのトリガーをチェック。押した瞬間だけtrueになる
//	bool IsTriggerMouse(const int32_t& buttonNumber);
//	// マウスの位置を取得する（ウィンドウ座標系）
//	const float2& GetMouseWindowPosition();
//	// マウスの位置を取得する（ウィンドウ座標系）
//	float2 GetMouseScreenPosition();
//	// 現在のジョイスティック状態を取得する
//	bool GetJoystickState(const int32_t& stickNo, XINPUT_STATE& out);
//	// 前回のジョイスティック状態を取得する
//	bool GetJoystickStatePrevious(const int32_t& stickNo, XINPUT_STATE& out);
//	// デッドゾーンを設定する
//	void SetJoystickDeadZone(const int32_t& stickNo, const int32_t& deadZoneL, const int32_t& deadZoneR);
//	// デッドゾーンを取得する
//	size_t GetNumberOfJoysticks();
//	// 接続されているジョイスティック数を取得する
//	bool IsTriggerPadButton(const PadButton& button, int32_t stickNo);
//	// 接続されているジョイスティック数を取得する
//	bool IsPressPadButton(const PadButton& button, int32_t stickNo);
//	// 接続されているジョイスティック数を取得する
//	float2 GetStickValue(const LR& padStick, int32_t stickNo);
//	// 接続されているジョイスティック数を取得する
//	float GetLRTrigger(const LR& LorR, int32_t stickNo);
//private:
//	friend class GameObject;
//	void Intialize(InputManager* input);
//	// 実装隠蔽クラス
//	class ImplInputAPI;
//	ImplInputAPI* implInputAPI = nullptr;
//	friend class GameObject;
//	InputManager* data = nullptr;
//};
//
//struct CHO_API EmitterAPI
//{
//public:
//	EmitterAPI();
//	~EmitterAPI();
//	// 関数ポインタのラッパー
//	void SetPosition(const float3& position);
//	void SetVelocity(const float3& velocity);
//	void SetAcceleration(const float3& acceleration);
//	void SetRotation(const float3& rotation);
//	void SetScale(const float3& scale);
//	void SetColor(const float3& color);
//	
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	// 実装隠蔽クラス
//	class ImplEmitterAPI;
//	ImplEmitterAPI* implEmitterAPI = nullptr;
//	EmitterComponent* data = nullptr;
//};
//
//struct CHO_API ParticleAPI
//{
//public:
//	ParticleAPI();
//	~ParticleAPI();
//	// 関数ポインタのラッパー
//	void Emit(const float3& position);
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	// 実装隠蔽クラス
//	class ImplParticleAPI;
//	ImplParticleAPI* implParticleAPI = nullptr;
//	//ParticleComponent* data = nullptr;
//	EmitterComponent* data = nullptr; 
//};
//
//struct CHO_API EffectAPI
//{
//public:
//	EffectAPI();
//	~EffectAPI();
//	// 関数ポインタのラッパー
//	void SetPosition(const float3& position);
//	void SetRotation(const float3& rotation);
//	void SetScale(const float3& scale);
//	void SetColor(const float3& color);
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	// 実装隠蔽クラス
//	class ImplEffectAPI;
//	ImplEffectAPI* implEffectAPI = nullptr;
//	EffectComponent* data = nullptr;
//};
//
//struct CHO_API UISpriteAPI
//{
//	// 関数
//	float2& position() { return data->position; }
//	float& rotation() { return data->rotation; }
//	float2& scale() { return data->scale; }
//	float2& anchorPoint() { return data->anchorPoint; }
//private:
//	friend class GameObject;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	UISpriteComponent* data = nullptr;
//};
//
//struct CHO_API EditorAPI
//{
//public:
//	EditorAPI();
//	~EditorAPI();
//	// 関数ポインタのラッパー
//	void Begin(const char* name);
//	void End();
//	void DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format = "%.3f");
//	void DragFloat2(const char* label, float* v[2], float v_speed, float v_min, float v_max, const char* format = "%.3f");
//	void DragFloat3(const char* label, float* v[3], float v_speed, float v_min, float v_max, const char* format = "%.3f");
//private:
//	friend class GameObject;
//	// 実装隠蔽クラス
//	class ImplEditorAPI;
//	ImplEditorAPI* implEditorAPI = nullptr;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//};
//
//struct CHO_API AudioAPI
//{
//public:
//	AudioAPI();
//	~AudioAPI();
//	// 関数
//	//void SetLoop(const bool& loop) { data->isLoop = loop; }
//	//bool GetIsPlay()const { return data->isPlay; }
//	// 関数ポインタのラッパー
//	void AddSource(const std::string& name);
//	void Play(const std::string& name, const bool& isLoop = false);
//	void Stop(const std::string& name);
//	bool IsPlaying(const std::string& name); // 再生中かどうかをチェック
//	void SetVolume(const std::string& name, const float& volume); // 音量を設定
//	//void SetSource(const std::string& sourceName);
//private:
//	friend class GameObject;
//	// 実装隠蔽クラス
//	class ImplAudioAPI;
//	ImplAudioAPI* implAudioAPI = nullptr;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	AudioComponent* data = nullptr;
//};
//
//struct CHO_API AnimationAPI
//{
//public:
//	AnimationAPI();
//	~AnimationAPI();
//	// 関数
//	void SetAnimationIndex(const int32_t& index) { data->animationIndex = index; }
//	// 関数ポインタのラッパー
//private:
//	friend class GameObject;
//	// 実装隠蔽クラス
//	class ImplAnimationAPI;
//	ImplAnimationAPI* implAnimationAPI = nullptr;
//	void Initialize(const Entity& entity, ECSManager* ecs, ObjectContainer* objectContainer, ResourceManager* resourceManager);
//	ECSManager* m_ECS = nullptr; // ECSManager
//	ObjectContainer* m_ObjectContainer = nullptr; // ObjectContainer
//	ResourceManager* m_ResourceManager = nullptr; // ResourceManager
//	Entity m_Entity; // Entity
//	AnimationComponent* data = nullptr;
//};

