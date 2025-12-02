#include "physicsengine_pch.h"
#include "2D/d2_world.h"
#include "2D/d2_body.h"
#include "2D/d2_converter.h"
// box2d
#include <box2d.h>

using namespace physics::d2;

inline bool operator==(const b2ShapeId& a, const b2ShapeId& b)
{
	return a.index1 == b.index1 &&
		a.world0 == b.world0 &&
		a.generation == b.generation;
}

inline bool operator<(const b2ShapeId& a, const b2ShapeId& b)
{
	if (a.index1 != b.index1) return a.index1 < b.index1;
	if (a.world0 != b.world0) return a.world0 < b.world0;
	return a.generation < b.generation;
}

namespace std
{
	template <>
	struct hash<b2ShapeId>
	{
		std::size_t operator()(const b2ShapeId& id) const noexcept
		{
			size_t h1 = std::hash<int32_t>{}(id.index1);
			size_t h2 = std::hash<uint16_t>{}(id.world0);
			size_t h3 = std::hash<uint16_t>{}(id.generation);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}

struct box2dWorld::Impl
{
	b2WorldId world;
	std::unordered_set<b2ShapeId> aliveShapes; // 有効な形状のセット
};

physics::d2::box2dWorld::box2dWorld(d2Backend be)
	: impl(std::make_unique<box2dWorld::Impl>())
{
	backend = be;	
}

physics::d2::box2dWorld::~box2dWorld()
{
	Destroy();
}

void physics::d2::box2dWorld::Create()
{
	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = b2Vec2(0.0f, -9.8f); // 重力の設定
	impl->world = b2CreateWorld(&worldDef); // Box2Dのワールドを作成
}

void physics::d2::box2dWorld::Destroy()
{
	b2DestroyWorld(impl->world); // Box2Dのワールドを破棄
}

Id2Body* physics::d2::box2dWorld::CreateBody(const Id2BodyDef& bodyDef)
{
	auto body = std::make_unique<box2dBody>();
	body->Create(this, bodyDef);
	Id2Body* raw = body.get();
	bodies.emplace_back(std::move(body));
	return raw;
}

void physics::d2::box2dWorld::DestroyBody(Id2Body* body)
{
	if (!body) return;
	std::erase_if(bodies, [body](const std::unique_ptr<Id2Body>& ptr) {
		return ptr.get() == body;
		});
}

void physics::d2::box2dWorld::Step(const float& deltaTime, const uint32_t& subStepCount)
{
	b2World_Step(impl->world, deltaTime, subStepCount); // Box2Dのワールドをステップ

	ProcessEvents();
}

Theatria::Math::float2 physics::d2::box2dWorld::GetGravity() const
{
	b2Vec2 gravity = b2World_GetGravity(impl->world);
	return Theatria::Math::float2(gravity.x, gravity.y); // Box2Dの重力を取得してTheatria::Math::float2に変換
}

void physics::d2::box2dWorld::SetGravity(const Theatria::Math::float2& gravity)
{
	b2Vec2 b2Gravity(gravity.x, gravity.y);
	b2World_SetGravity(impl->world, b2Gravity); // Box2Dの重力を設定
}

b2WorldId physics::d2::box2dWorld::GetWorld() const { return impl->world; }

// Box2Dのワールドを取得
void physics::d2::box2dWorld::ProcessEvents()
{
	b2ContactEvents events = b2World_GetContactEvents(impl->world);

	// 事前にクリア
	currentContacts.clear();

	// BeginContact
	for(int i = 0; i < events.beginCount; ++i)
	{
		const b2ContactBeginTouchEvent& ev = events.beginEvents[i];
		auto pair = std::minmax(ev.shapeIdA, ev.shapeIdB);// ソートして重複防止
		currentContacts.insert(pair);

		if (beginContactCallback)
		{
			auto a = b2Body_GetUserData(b2Shape_GetBody(ev.shapeIdA));
			auto b = b2Body_GetUserData(b2Shape_GetBody(ev.shapeIdB));
			beginContactCallback(a, b);
		}
	}
	// EndContact
	for(int i = 0; i < events.endCount; ++i)
	{
		const b2ContactEndTouchEvent& ev = events.endEvents[i];
		auto pair = std::minmax(ev.shapeIdA, ev.shapeIdB);
		// EndはStayでも処理しない
		if (endContactCallback)
		{
			auto a = b2Body_GetUserData(b2Shape_GetBody(ev.shapeIdA));
			auto b = b2Body_GetUserData(b2Shape_GetBody(ev.shapeIdB));
			endContactCallback(a, b);
		}
	}
	// Stay 判定（previous にあって current にもあるもの）
	if (stayContactCallback)
	{
		for (const auto& pair : previousContacts)
		{
			if (currentContacts.count(pair))
			{
				if (impl->aliveShapes.count(pair.first) && impl->aliveShapes.count(pair.second))
				{
					auto a = b2Body_GetUserData(b2Shape_GetBody(pair.first));
					auto b = b2Body_GetUserData(b2Shape_GetBody(pair.second));
					stayContactCallback(a, b);
				}
			}
		}
	}

	// 次回比較用に保存
	std::swap(previousContacts, currentContacts);
}

void physics::d2::box2dWorld::InsertShapeId(const b2ShapeId& shapeId)
{
	impl->aliveShapes.insert(shapeId); // 有効な形状のセットに追加
}

void physics::d2::box2dWorld::RemoveShapeId(const b2ShapeId& shapeId)
{
	impl->aliveShapes.erase(shapeId); // 有効な形状のセットから削除
}

struct choPhysicsWorld::Impl
{
	int dummy = 0; // ChoPhysicsの実装に必要なダミー変数
};

physics::d2::choPhysicsWorld::choPhysicsWorld(d2Backend be)
	: impl(std::make_unique<choPhysicsWorld::Impl>())
{
	backend = be;
}

void physics::d2::choPhysicsWorld::Create()
{
}

void physics::d2::choPhysicsWorld::Destroy()
{
}

void physics::d2::choPhysicsWorld::Step(const float& deltaTime, const uint32_t& subStepCount)
{
	deltaTime; subStepCount;
	ProcessEvents();
}

Theatria::Math::float2 physics::d2::choPhysicsWorld::GetGravity() const
{
	return Theatria::Math::float2();
}

void physics::d2::choPhysicsWorld::SetGravity(const Theatria::Math::float2& gravity)
{
	gravity;
}
