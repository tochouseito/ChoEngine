#include "pch.h"
#include "GameObject.h"
#include "GameCore/ECS/ECSManager.h"
#include "Platform/InputManager/InputManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

class GameObject::ImplGameObject
{
public:
	ImplGameObject() = default;
	~ImplGameObject()
	{
		m_SrcEntity = std::nullopt; // クローン時のオリジナルのIDをクリア
		m_SceneName = L""; // 管理しているSceneNameをクリア
		// parameters.clear(); // スクリプト用パラメータをクリア
		m_Name = L""; // ゲームオブジェクト名をクリア
	}
	std::function<std::optional<Entity>()> GetEntityFunc;	// スクリプト用ゲームオブジェクトID取得関数
	std::function<void(const std::optional<Entity>& id)> SetEntityFunc;	// スクリプト用ゲームオブジェクトID設定関数
	std::function<std::wstring()> GetNameFunc;	// スクリプト用ゲームオブジェクト名取得関数
	std::function<void(const std::wstring& name)> SetNameFunc;	// スクリプト用ゲームオブジェクト名設定関数
	std::function<std::string()> GetTagFunc;	// スクリプト用タグ取得関数
	std::function<void(const std::string& tag)> SetTagFunc;	// スクリプト用タグ設定関数
	std::function<ObjectParameter(const std::string& name)> GetParameterFunc;	// スクリプト用パラメータ取得関数
	std::function<void(const std::string& name, const ObjectParameter& value)> SetParameterFunc;	// スクリプト用パラメータ設定関数
	// ゲームオブジェクト名
	std::wstring m_Name = L"";
	// タグ
	std::string m_Tag = "Default";
	// スクリプト用パラメータ
	std::unordered_map<std::string, ObjectParameter> parameters;
	// 管理しているSceneName
	std::wstring m_SceneName = L"";
	// クローン時のオリジナルのID
	std::optional<Entity> m_SrcEntity = std::nullopt;
};

std::optional<Entity> GameObject::GetSrcEntity() const noexcept
{
	if(implGameObject)
	{
		return implGameObject->m_SrcEntity;
	}
	return std::nullopt;
}

std::wstring GameObject::GetName() const noexcept
{
	if (implGameObject && implGameObject->GetNameFunc)
	{
		return implGameObject->GetNameFunc();
	}
	return L"";
}

std::string GameObject::GetTag() const noexcept
{
	if (implGameObject && implGameObject->GetTagFunc)
	{
		return implGameObject->GetTagFunc();
	}
	return "Default";
}

std::wstring GameObject::GetCurrentSceneName() const noexcept
{
	if (implGameObject)
	{
		return implGameObject->m_SceneName;
	}
	return L"";
}

void GameObject::SetTag(std::string_view tag) noexcept
{
	if (implGameObject && implGameObject->SetTagFunc)
	{
		implGameObject->SetTagFunc(tag.data());
	}
	return;
}

ObjectParameter GameObject::GetParameter(const std::string& name) const
{
	if (implGameObject && implGameObject->GetParameterFunc)
	{
		return implGameObject->GetParameterFunc(name);
	}
	return ObjectParameter();
}

void GameObject::SetParameter(const std::string& name, const ObjectParameter& value)
{
	if (implGameObject && implGameObject->SetParameterFunc)
	{
		implGameObject->SetParameterFunc(name, value);
	}
}

void GameObject::SetName(const std::wstring& name) noexcept
{
	if (implGameObject)
	{
		implGameObject->SetNameFunc(name);
	}
}

void GameObject::SetCurrentSceneName(const std::wstring& name) noexcept
{
	if (implGameObject)
	{
		implGameObject->m_SceneName = name;
	}
}

ScriptComponent* GameObject::GetScriptComponent() const noexcept
{
	return m_ECS->GetComponent<ScriptComponent>(m_Handle.entity);
}

GameObject::GameObject(ECSManager* ecsManager,const ObjectHandle& handle, const std::wstring& name, const ObjectType& type,const theatriaSystem::Transform& tf) :
	m_ECS(ecsManager), m_Handle(handle), m_Type(type), transform(tf)
{
	m_Active = true;
	implGameObject = new ImplGameObject();
	implGameObject->GetNameFunc = [this]() {
		return implGameObject->m_Name;
		};
	implGameObject->SetNameFunc = [this](const std::wstring& name) {
		implGameObject->m_Name = name;
		};
	implGameObject->GetTagFunc = [this]() {
		return implGameObject->m_Tag;
		};
	implGameObject->SetTagFunc = [this](const std::string& tag) {
		implGameObject->m_Tag = tag;
		};
	implGameObject->GetParameterFunc = [this](const std::string& name) {
		ObjectParameter value;
		value = implGameObject->parameters[name];
		return value;
		};
	implGameObject->SetParameterFunc = [this](const std::string& name, const ObjectParameter& value) {
		implGameObject->parameters[name] = value;
		};
	// ゲームオブジェクトの名前を設定
	implGameObject->SetNameFunc(name);
}

GameObject::~GameObject()
{
	if (implGameObject)
	{
		delete implGameObject;
		implGameObject = nullptr;
	}
}
