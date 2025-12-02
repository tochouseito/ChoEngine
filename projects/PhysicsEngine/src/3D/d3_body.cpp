#include "physicsengine_pch.h"
#include "3D/d3_body.h"
#include "3D/d3_world.h"

// BulletPhysics
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

using namespace physics::d3;

struct bulletBody::Impl
{
	std::unique_ptr<btCollisionShape> shape; // Bulletの衝突形状データ
	std::unique_ptr<btDefaultMotionState> motionState; // 物理オブジェクトの状態
	std::unique_ptr<btRigidBody> rigidBody; // 剛体データ
	Id3BodyType currentType = Id3BodyType::DYNAMIC; // 現在のボディタイプ
	float lastDynamicMass = 1.0f; // 最後に設定した質量
};

physics::d3::bulletBody::bulletBody():
	impl(std::make_unique<Impl>())
{
}

void physics::d3::bulletBody::Create(const Id3BodyDef& bodyDef)
{
	impl->shape = std::make_unique<btBoxShape>(btVector3(bodyDef.halfsize.x, bodyDef.halfsize.y, bodyDef.halfsize.z));
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(bodyDef.position.x, bodyDef.position.y, bodyDef.position.z));

	// degreesからクォータニオンを作成
	btScalar roll = btRadians(bodyDef.degrees.x);
	btScalar pitch = btRadians(bodyDef.degrees.y);
	btScalar yaw = btRadians(bodyDef.degrees.z);
	btQuaternion quat;
	quat.setEulerZYX(yaw, pitch, roll); // ZYX順で設定
	transform.setRotation(quat);

	impl->motionState = std::make_unique<btDefaultMotionState>(transform);
	btScalar mass = bodyDef.mass; // 質量を設定
	btVector3 inertia(0, 0, 0);
	if(mass != 0.0f)
	{
		impl->shape->calculateLocalInertia(mass, inertia);
	}
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, impl->motionState.get(), impl->shape.get(), inertia);
	impl->rigidBody = std::make_unique<btRigidBody>(rbInfo);
	impl->rigidBody->setFriction(bodyDef.friction);
	impl->rigidBody->setRestitution(bodyDef.restitution);
	impl->rigidBody->setUserPointer(bodyDef.userData); // ユーザーデータを設定
	impl->rigidBody->setUserIndex(bodyDef.userIndex); // ユーザーインデックスを設定
	impl->currentType = bodyDef.bodyType;
}

void physics::d3::bulletBody::Destroy()
{
	impl->motionState.reset(); // モーションステートの解放
	impl->rigidBody.reset(); // 剛体の解放
	impl->shape.reset(); // 衝突形状の解放
}

Theatria::Math::float3 physics::d3::bulletBody::GetPosition() const
{
	btTransform transform;
	impl->rigidBody->getMotionState()->getWorldTransform(transform);
	return Theatria::Math::float3(transform.getOrigin().x(), transform.getOrigin().y(), transform.getOrigin().z());
}

Theatria::Math::Quaternion physics::d3::bulletBody::GetQuaternion() const
{
	btTransform transform;
	impl->rigidBody->getMotionState()->getWorldTransform(transform);
	return Theatria::Math::Quaternion(transform.getRotation().x(), transform.getRotation().y(), transform.getRotation().z(), transform.getRotation().w());
}

Theatria::Math::float3 physics::d3::bulletBody::GetRotation() const
{
	btTransform transform = impl->rigidBody->getWorldTransform();
	btQuaternion quat = transform.getRotation();
	btMatrix3x3 mat(quat);  // クォータニオンを回転行列に変換
	btScalar roll, pitch, yaw;
	mat.getEulerYPR(yaw, pitch, roll); // 引数は (yaw, pitch, roll) の順
	btScalar degreesX, degreesY, degreesZ;
	degreesX = btDegrees(roll);
	degreesY = btDegrees(pitch);
	degreesZ = btDegrees(yaw);
	return Theatria::Math::float3(degreesX, degreesY, degreesZ);
}

void physics::d3::bulletBody::SetTransform(const Theatria::Math::float3& position, const Theatria::Math::Quaternion& rotation)
{
	btTransform transform = impl->rigidBody->getWorldTransform();
	transform.setOrigin(btVector3(position.x, position.y, position.z));
	transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
	impl->rigidBody->getMotionState()->setWorldTransform(transform);
	impl->rigidBody->setWorldTransform(transform); // 剛体のワールド変換を更新
}

void physics::d3::bulletBody::SetTransform(const Theatria::Math::float3& position)
{
	btTransform transform = impl->rigidBody->getWorldTransform();
	transform.setOrigin(btVector3(position.x, position.y, position.z));
	impl->rigidBody->getMotionState()->setWorldTransform(transform);
	impl->rigidBody->setWorldTransform(transform); // 剛体のワールド変換を更新
}

void physics::d3::bulletBody::SetTransform(const Theatria::Math::Quaternion& rotation)
{
	btTransform transform = impl->rigidBody->getWorldTransform();
	transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
	impl->rigidBody->getMotionState()->setWorldTransform(transform);
	impl->rigidBody->setWorldTransform(transform); // 剛体のワールド変換を更新
}

Theatria::Math::float3 physics::d3::bulletBody::GetLinearVelocity() const
{
	btVector3 linVel = impl->rigidBody->getLinearVelocity();
	return Theatria::Math::float3(linVel.x(), linVel.y(), linVel.z());
}

void physics::d3::bulletBody::SetLinearVelocity(const Theatria::Math::float3& velocity)
{
	impl->rigidBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
}

Theatria::Math::float3 physics::d3::bulletBody::GetAngularVelocity() const 
{ 
	btVector3 angVel = impl->rigidBody->getAngularVelocity();
	return Theatria::Math::float3(angVel.x(), angVel.y(), angVel.z());
}

void physics::d3::bulletBody::SetAngularVelocity(const Theatria::Math::float3& angularVelocity)
{
	impl->rigidBody->setAngularVelocity(btVector3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
}

void physics::d3::bulletBody::SetGravityScale(Id3World* world, const float& scale)
{
	if (!impl || !impl->rigidBody || !world) return;
	bulletWorld* bulletWorldPtr = static_cast<bulletWorld*>(world);
	// ワールド重力に上書きされないようにフラグ設定
	impl->rigidBody->setFlags(impl->rigidBody->getFlags() | BT_DISABLE_WORLD_GRAVITY);
	// スケール適用した重力を設定
	impl->rigidBody->setGravity(bulletWorldPtr->GetDynamicsWorld()->getGravity() * scale);
	// 再アクティブ化
	impl->rigidBody->activate(true);
}

void physics::d3::bulletBody::SetLinearFactor(const Theatria::Math::float3& factor)
{
	if (!impl || !impl->rigidBody) return;
	impl->rigidBody->setLinearFactor(btVector3(factor.x, factor.y, factor.z));
}

void physics::d3::bulletBody::SetAngularFactor(const Theatria::Math::float3& factor)
{
	if (!impl || !impl->rigidBody) return;
	impl->rigidBody->setAngularFactor(btVector3(factor.x, factor.y, factor.z));
}

btRigidBody* physics::d3::bulletBody::GetRigidBody() const { return impl->rigidBody.get(); }

// Bulletの剛体データを取得
bool physics::d3::bulletBody::IsActive() const { return impl->rigidBody->isActive(); }

// 有効かどうかを取得
void physics::d3::bulletBody::SetActive(bool active) { impl->rigidBody->activate(active); }

void physics::d3::bulletBody::SetBodyType(Id3World* world, Id3BodyType bodyType)
{
	if (!impl || !impl->rigidBody || !world) return;

	btRigidBody* body = impl->rigidBody.get();
	btCollisionShape* shape = impl->shape.get();
	bulletWorld* bulletWorldPtr = static_cast<bulletWorld*>(world);
	btDynamicsWorld* worldPtr = bulletWorldPtr->GetDynamicsWorld();
	if (impl->currentType == bodyType) return;

	auto flags = body->getCollisionFlags();

	// DYNAMIC のときの質量を覚えておく（切替元がDYNAMICなら）
	if (impl->currentType == Id3BodyType::DYNAMIC)
	{
		btScalar invMass = body->getInvMass();
		if (invMass > 0)
		{
			impl->lastDynamicMass = btScalar(1.0f) / invMass; // 現在の質量を保存
		}
	}

	switch (bodyType)
	{
	case Id3BodyType::STATIC:
	{
		// Kinematic フラグを外す
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
		body->setCollisionFlags(flags);

		// 質量0 / 慣性0 → Static 扱い
		btVector3 inertia(0, 0, 0);
		body->setMassProps(btScalar(0.0f), inertia);
		body->updateInertiaTensor();

		// 速度をゼロに
		body->setLinearVelocity(btVector3(0, 0, 0));
		body->setAngularVelocity(btVector3(0, 0, 0));

		// スリープ許可
		body->setActivationState(WANTS_DEACTIVATION);

		// AABB 更新（位置を動かすStaticなら特に重要）
		if (worldPtr) worldPtr->updateSingleAabb(body);
		break;
	}

	case Id3BodyType::DYNAMIC:
	{
		// Kinematic フラグを外す
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
		body->setCollisionFlags(flags);

		// 質量を復元（最低1.0fなど下限を決めると安定）
		btScalar remakeMass = btMax(impl->lastDynamicMass, btScalar(1.0f));

		btVector3 inertia(0, 0, 0);
		if (shape && remakeMass > 0)
		{
			shape->calculateLocalInertia(remakeMass, inertia);
		}
		body->setMassProps(remakeMass, inertia);
		body->updateInertiaTensor();

		// 再アクティブ化
		body->setActivationState(ACTIVE_TAG);
		body->activate(true);

		if (worldPtr) worldPtr->updateSingleAabb(body);
		break;
	}

	case Id3BodyType::KINEMATIC:
	{
		// Kinematic フラグON（質量は0にする）
		flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
		body->setCollisionFlags(flags);

		btVector3 inertia(0, 0, 0);
		body->setMassProps(btScalar(0.0f), inertia);
		body->updateInertiaTensor();

		// 速度は基本ゼロに（毎フレーム手動更新で与える）
		body->setLinearVelocity(btVector3(0, 0, 0));
		body->setAngularVelocity(btVector3(0, 0, 0));

		// スリープ無効化（手で動かす壁なので寝かせない）
		body->setActivationState(DISABLE_DEACTIVATION);

		if (worldPtr) worldPtr->updateSingleAabb(body);
		break;
	}

	default:
		break;
	}

	impl->currentType = bodyType;
}

void physics::d3::bulletBody::SetSensor(bool isSensor)
{
	// センサーオブジェクトの設定
	if (isSensor)
	{
		impl->rigidBody->setCollisionFlags(impl->rigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}
	else
	{
		impl->rigidBody->setCollisionFlags(impl->rigidBody->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}
}

void physics::d3::chophysicsBody::Create(const Id3BodyDef&)
{
}

void physics::d3::chophysicsBody::Destroy()
{
}

Theatria::Math::float3 physics::d3::chophysicsBody::GetPosition() const
{
	return Theatria::Math::float3();
}

Theatria::Math::Quaternion physics::d3::chophysicsBody::GetQuaternion() const
{
	return Theatria::Math::Quaternion();
}

Theatria::Math::float3 physics::d3::chophysicsBody::GetLinearVelocity() const
{
	return Theatria::Math::float3();
}

void physics::d3::chophysicsBody::SetLinearVelocity(const Theatria::Math::float3&)
{
}

bool physics::d3::chophysicsBody::IsActive() const
{
	return false;
}
