#pragma once
#include <string>
#include "Core/Utility/IDType.h"
#include "GameCore/ScriptAPI/ScriptAPI.h"


using ObjectParameter = std::variant<int, float, bool, float3>;
class ECSManager;
class ResourceManager;

// GameObjectクラス
class THEATRIA_API GameObject
{
public:
	std::optional<Entity> GetSrcEntity() const noexcept;
	ObjectHandle GetHandle() const noexcept { return m_Handle; }
	void SetHandle(const ObjectHandle& handle) noexcept { m_Handle = handle; }
	std::wstring GetName() const noexcept;
	ObjectType GetType() const noexcept { return m_Type; }
	std::string GetTag() const noexcept;
	std::wstring GetCurrentSceneName() const noexcept;
	void SetCurrentSceneName(const std::wstring& name) noexcept;
	void SetTag(std::string_view tag) noexcept;// この関数はEditorに移す予定
	void SetType(const ObjectType& type) noexcept { m_Type = type; }
	void SetName(const std::wstring& name) noexcept;
	// オブジェクトが有効かどうか
	bool IsActive() const noexcept { return m_Active; }
	// オブジェクトを有効にする
	void SetActive(bool active) noexcept { m_Active = active; }
	// オブジェクトを無効にする
	void SetInactive() noexcept { m_Active = false; }
	// スクリプトのインスタンスを取得
	template<typename T>
	T* GetMarionnette() const noexcept
	{
		// TがMarionnetteを継承しているか確認
		static_assert(std::is_base_of<Marionnette, T>::value, "T must be derived from Marionnette");
		if (ScriptComponent* script = GetScriptComponent())
		{
			return static_cast<T*>(script->instance);
		}
		return nullptr;
	}
	// パラメータを取得
	ObjectParameter GetParameter(const std::string& name) const;
	// パラメータを設定
	void SetParameter(const std::string& name, const ObjectParameter& value);
public:
	theatriaSystem::Transform transform;
private:
	ObjectHandle m_Handle;			// オブジェクトハンドル
	ObjectType m_Type;								// ゲームオブジェクトのタイプ
	bool m_Active = false;							// アクティブフラグ

	// 実装隠蔽クラス
	class ImplGameObject;
	ImplGameObject* implGameObject = nullptr;

	// ECSManagerへのポインタ
	ECSManager* m_ECS = nullptr;

	// スクリプトインスタンス取得関数補助
	ScriptComponent* GetScriptComponent() const noexcept;
public:
	// コンストラクタ
	GameObject(
		ECSManager* ecsManager,
		const ObjectHandle& handle,
		const std::wstring& name,
		const ObjectType& type,
		const theatriaSystem::Transform& tf);
	// デストラクタ
	~GameObject();
	// コピー、代入禁止
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	// ムーブは許可する
	GameObject(GameObject&&) noexcept = default;
	GameObject& operator=(GameObject&&) noexcept = default;
};
