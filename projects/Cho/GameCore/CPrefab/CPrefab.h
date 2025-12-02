#pragma once
#include "GameCore/GameObject/GameObject.h"
#include "GameCore/ECS/ECSManager.h"
class CPrefab : public IPrefab
{
public:
	CPrefab(const std::wstring& name, ObjectType type,const std::string& tag = "Default")
		: m_Name(name), m_Type(type), m_Tag(tag)
	{
	}
	~CPrefab() = default;

	std::wstring GetName() const { return m_Name; }
	ObjectType GetType() const { return m_Type; }
	std::string GetTag() const { return m_Tag; }
	static CPrefab FromEntity(ECSManager& ecs, Entity e, const std::wstring& name, ObjectType type, const std::string& tag = "Default")
	{
		CPrefab prefab(name, type,tag);
		prefab.PopulateFromEntity(ecs, e);
		return prefab;
	}
private:
	Entity CreateEntity(ECSManager& ecs) const override
	{
		Entity e = IPrefab::CreateEntity(ecs);
		return e;
	}

	std::wstring m_Name;
	ObjectType m_Type;
	std::string m_Tag;
};

