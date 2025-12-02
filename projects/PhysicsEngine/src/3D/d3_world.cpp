#include "physicsengine_pch.h"
#include "3D/d3_world.h"
#include "3D/d3_body.h"
// BulletPhysics
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

using namespace physics::d3;

namespace std
{
	template <>
	struct hash<std::pair<const btCollisionObject*, const btCollisionObject*>>
	{
		std::size_t operator()(const std::pair<const btCollisionObject*, const btCollisionObject*>& p) const noexcept
		{
			return reinterpret_cast<std::uintptr_t>(p.first) ^ (reinterpret_cast<std::uintptr_t>(p.second) << 1);
		}
	};
}

struct bulletWorld::Impl
{
	// Bulletのオブジェクト定義
	std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig = nullptr;// 衝突設定
	std::unique_ptr<btCollisionDispatcher> dispatcher = nullptr;// 衝突ディスパッチャー
	std::unique_ptr<btBroadphaseInterface> broadphase = nullptr;// ブロードフェーズ
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver = nullptr;// 制約ソルバー
	std::unique_ptr<btDiscreteDynamicsWorld> world = nullptr;// 物理ワールド

	std::unordered_set<std::pair<const btCollisionObject*, const btCollisionObject*>> currentContacts;
	std::unordered_set<std::pair<const btCollisionObject*, const btCollisionObject*>> previousContacts;

	void RemoveContactsOf(const btCollisionObject* obj)
	{
		// 現在の接触から指定されたオブジェクトとの接触を削除
		auto erase_from = [obj](auto& s) {
			for (auto it = s.begin(); it != s.end();)
			{
				if(it->first == obj || it->second == obj)
				{
					it = s.erase(it);
				}
				else
				{
					++it;
				}
			}
			};
		erase_from(currentContacts);
		erase_from(previousContacts);
	}
};

physics::d3::bulletWorld::bulletWorld():
	impl(std::make_unique<Impl>())
{
	// バックエンドを設定
	backend = d3Backend::bullet;
	// 衝突構成、検出の初期化
	impl->collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
	impl->dispatcher = std::make_unique<btCollisionDispatcher>(impl->collisionConfig.get());
	impl->broadphase = std::make_unique<btDbvtBroadphase>();
	impl->solver = std::make_unique<btSequentialImpulseConstraintSolver>();
	// 物理ワールドの初期化
	impl->world = std::make_unique<btDiscreteDynamicsWorld>(
		impl->dispatcher.get(),
		impl->broadphase.get(),
		impl->solver.get(),
		impl->collisionConfig.get()
	);
	impl->world->setGravity(btVector3(0, 0.0f, 0)); // 重力を設定
}

physics::d3::bulletWorld::~bulletWorld()
{
	// 物理ワールドの解放
	impl->world.reset();
	impl->solver.reset();
	impl->broadphase.reset();
	impl->dispatcher.reset();
	impl->collisionConfig.reset();
	// implの解放
	impl.reset();
}

// 重力の設定、取得
Theatria::Math::float3 physics::d3::bulletWorld::GetGravity() const
{
	btVector3 gravity = impl->world->getGravity();
	return Theatria::Math::float3(gravity.x(), gravity.y(), gravity.z());
}

void physics::d3::bulletWorld::SetGravity(const Theatria::Math::float3& gravity)
{
	impl->world->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}

void physics::d3::bulletWorld::Step(const float& deltaTime)
{
	impl->world->stepSimulation(deltaTime);
	if (deltaTime != 0.0f)
	{
		ProcessEvents();
	}
	else
	{
		impl->previousContacts.clear(); // ゼロステップの場合は前回の接触をクリア
		impl->currentContacts.clear(); // ゼロステップの場合は現在の接触もクリア
	}
}

Id3Body* physics::d3::bulletWorld::CreateBody(const Id3BodyDef& bodyDef)
{
	auto body = std::make_unique<bulletBody>();
	body->Create(bodyDef);
	impl->world->addRigidBody(body->GetRigidBody()); // Bulletのワールドにボディを追加
	Id3Body* raw = body.get();
	bodies.emplace_back(std::move(body));
	return raw;
}

void physics::d3::bulletWorld::DestroyBody(Id3Body* body)
{
	if (!body) return;
	bulletBody* bulletBodyPtr = static_cast<bulletBody*>(body);
	btRigidBody* rb = bulletBodyPtr->GetRigidBody();
	// 削除する前に衝突履歴から削除
	impl->RemoveContactsOf(rb);
	impl->world->removeRigidBody(rb);
	bulletBodyPtr->Destroy();
	std::erase_if(bodies, [body](const std::unique_ptr<Id3Body>& ptr) {
		return ptr.get() == body;
		});
}

void physics::d3::bulletWorld::ProcessEvents()
{
	using Pair = std::pair<const btCollisionObject*, const btCollisionObject*>;
	impl->currentContacts.clear();

	int numManifolds = impl->world->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; ++i)
	{
		btPersistentManifold* manifold = impl->world->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject* a = static_cast<const btCollisionObject*>(manifold->getBody0());
		const btCollisionObject* b = static_cast<const btCollisionObject*>(manifold->getBody1());

		if (manifold->getNumContacts() > 0)
		{
			Pair pair = std::minmax(a, b);
			impl->currentContacts.insert(pair);

			if (!impl->previousContacts.contains(pair) && beginContactCallback)
			{
				beginContactCallback(a->getUserIndex(), b->getUserIndex());
			}
		}
	}

	for (const Pair& pair : impl->previousContacts)
	{
		if (!impl->currentContacts.contains(pair))
		{
			if (endContactCallback)
				endContactCallback(pair.first->getUserIndex(), pair.second->getUserIndex());
		}
		else if (stayContactCallback)
		{
			stayContactCallback(pair.first->getUserIndex(), pair.second->getUserIndex());
		}
	}

	impl->previousContacts = impl->currentContacts;
}

btDynamicsWorld* physics::d3::bulletWorld::GetDynamicsWorld() const { return impl->world.get(); }

Id3World* physics::d3::CreateWorld(d3Backend backend)
{
	switch (backend)
	{
	case physics::d3::d3Backend::bullet:
		return new bulletWorld();
		break;
	case physics::d3::d3Backend::chophysics:
		return new chophysicsWorld();
		break;
	default:
		return nullptr; // 未対応のバックエンド
		break;
	}
}

void physics::d3::DestroyWorld(Id3World* world) noexcept
{
	if (world)
	{
		delete world;
	}
}
