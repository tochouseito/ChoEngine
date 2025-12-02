#include "physicsengine_pch.h"
#include "3D/d3_shape.h"
#include "3D/d3_world.h"

// BulletPhysics
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

using namespace physics::d3;

struct bulletShape::Impl
{
	std::unique_ptr<btCollisionShape> shape; // Bulletの衝突形状データ
	std::unique_ptr<btDefaultMotionState> motionState; // 物理オブジェクトの状態
	std::unique_ptr<btRigidBody> rigidBody; // 剛体データ
};
