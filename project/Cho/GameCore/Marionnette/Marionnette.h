#pragma once
#include "GameCore/GameObject/GameObject.h"
#include "Core/Utility/Components.h"
#define USE_THEATRIAENGINE_SCRIPT
#include "TheatriaEngineAPI.h"
#include "TheaReflectDeclare.h"
namespace math = Theatria::Math;
class THEATRIA_API Marionnette
{
    friend class ScriptInstanceGenerateSystem;
    friend class ScriptSystem;
public:
    Marionnette(GameObject& object) : gameObject(object),transform(object.GetHandle().entity, nullptr)
    {
	}
    virtual ~Marionnette() = default;
    virtual void Start() = 0;
    virtual void Update() = 0;
    virtual void OnCollisionEnter(GameObject&) {}
    virtual void OnCollisionStay(GameObject&) {}
    virtual void OnCollisionExit(GameObject&) {}
    template<theatriaSystem::Type T>
    T GetComponent() const
    {
        T comp(gameObject.GetHandle().entity, ecsManager);
        return comp;
    }
public:
	GameObject& gameObject;
	theatriaSystem::Transform transform;
private:
	void SetECSPtr(ECSManager* ecs) 
    { 
        ecsManager = ecs;
		transform.m_ECS = ecs;
    }
	ECSManager* ecsManager = nullptr;
};
