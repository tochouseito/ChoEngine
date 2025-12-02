#include "physicsengine_pch.h"
#include "2D/d2_body.h"
#include "2D/d2_world.h"
#include "2D/d2_converter.h"
#include "2D/d2_shape.h"

// box2d
#include <box2d.h>

using namespace physics::d2;

struct box2dBody::Impl
{
	b2BodyId body;
	b2ShapeId* pShapeId = nullptr; // 形状IDのポインタ
};

physics::d2::box2dBody::box2dBody()
	: impl(std::make_unique<box2dBody::Impl>())
{
}

physics::d2::box2dBody::~box2dBody()
{
	Destroy();
}

void physics::d2::box2dBody::Create(Id2World* world, const Id2BodyDef& bodyDef)
{
	pWorld = static_cast<box2dWorld*>(world); // 所属するワールドを設定
	b2BodyDef b2BodyDef = b2DefaultBodyDef();
	b2BodyDef.userData = bodyDef.userData; // ユーザーデータの設定
	b2BodyDef.type = ConvertBodyType(bodyDef.type); // ボディタイプの変換
	b2BodyDef.gravityScale = bodyDef.gravityScale; // 重力スケールの設定
	b2BodyDef.fixedRotation = bodyDef.fixedRotation; // 回転の固定設定
	b2BodyDef.position = b2Vec2(bodyDef.position.x, bodyDef.position.y); // 位置の設定
	b2BodyDef.rotation = b2MakeRot(bodyDef.angle); // 角度の設定
	impl->body = b2CreateBody(pWorld->GetWorld(), &b2BodyDef); // Box2Dのボディを作成
	isActive = true; // 有効フラグを設定
}

void physics::d2::box2dBody::Destroy()
{
	if(impl->pShapeId)
	{
		pWorld->RemoveShapeId(*impl->pShapeId); // ワールドから形状IDを削除
		impl->pShapeId = nullptr; // ポインタをnullptrに設定
	}
	b2DestroyBody(impl->body);
	isActive = false; // 有効フラグを無効に設定
}

Id2Shape* physics::d2::box2dBody::CreateShape(Id2ShapeDef* shapeDef, Id2Polygon* polygon)
{
	m_Shape = std::make_unique<box2dShape>();
	m_Shape->CreatePolygonShape(this, shapeDef, polygon); // 形状を作成
	Id2Shape* rawShape = m_Shape.get();
	return rawShape; // 生のポインタを返す
}

void physics::d2::box2dBody::DestroyShape()
{
	if (m_Shape)
	{
		m_Shape->Destroy(); // 形状を削除
		m_Shape.reset(); // 形状のポインタをリセット
	}
}

void physics::d2::box2dBody::SetAwake(bool flag)
{
	b2Body_SetAwake(impl->body, flag); // ボディを起こす
}

Theatria::Math::float2 physics::d2::box2dBody::GetPosition() const
{
	b2Vec2 pos = b2Body_GetPosition(impl->body);
	return Theatria::Math::float2(pos.x, pos.y);
}

Theatria::Math::float2 physics::d2::box2dBody::GetLinearVelocity() const
{
	b2Vec2 velocity = b2Body_GetLinearVelocity(impl->body);
	Theatria::Math::float2 result(velocity.x, velocity.y);
	return result;
}

void physics::d2::box2dBody::SetLinearVelocity(const Theatria::Math::float2& velocity)
{
	b2Body_SetLinearVelocity(impl->body, b2Vec2(velocity.x, velocity.y));
}

void physics::d2::box2dBody::SetTransform(const Theatria::Math::float2& position, const float& angle)
{
	b2Body_SetTransform(impl->body , b2Vec2(position.x, position.y), b2MakeRot(angle));
}

float physics::d2::box2dBody::GetAngle() const { return  b2Rot_GetAngle(b2Body_GetRotation(impl->body)); }

b2BodyId physics::d2::box2dBody::GetBody()
{
	return impl->body;
}

box2dWorld* physics::d2::box2dBody::GetWorld() const
{
	return pWorld; // 所属するワールドを取得
}

void physics::d2::box2dBody::SetShapeId(b2ShapeId* shapeId) { impl->pShapeId = shapeId; }

// 形状IDを設定
void physics::d2::box2dBody::RemoveShapeId() { impl->pShapeId = nullptr; }
