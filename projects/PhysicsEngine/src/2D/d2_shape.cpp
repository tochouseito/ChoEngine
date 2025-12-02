#include "physicsengine_pch.h"
#include "2D/d2_shape.h"
#include "2D/d2_body.h"
#include "2D/d2_polygon.h"
#include "2D/d2_world.h"

#include <box2d.h>

using namespace physics::d2;

struct box2dShape::Impl
{
	b2ShapeId shape; // Box2Dの形状
	box2dBody* pBody = nullptr; // 所属するボディ
};

physics::d2::box2dShape::box2dShape()
	: impl(std::make_unique<box2dShape::Impl>())
{
}

void physics::d2::box2dShape::CreatePolygonShape(Id2Body* body, Id2ShapeDef* shapeDef, Id2Polygon* polygon)
{
	box2dBody* box2dBodyPtr = static_cast<box2dBody*>(body);
	impl->pBody = box2dBodyPtr; // 所属するボディを設定
	pWorld = box2dBodyPtr->GetWorld(); // 所属するワールドを設定
	std::unique_ptr<box2dPolygon> box2dPolygonPtr = std::make_unique<box2dPolygon>();
	float width = polygon->GetSize().x; // ポリゴンの幅を取得
	float height = polygon->GetSize().y; // ポリゴンの高さを取得
	box2dPolygonPtr->MakeBox(width * 0.5f, height * 0.5f); // ポリゴンのサイズを設定
	b2ShapeDef def = b2DefaultShapeDef();
	def.userData = shapeDef->userData; // ユーザーデータの設定
	def.density = shapeDef->density; // 密度の設定
	def.material.friction = shapeDef->friction; // 摩擦係数の設定
	def.material.restitution = shapeDef->restitution; // 反発係数の設定
	def.isSensor = shapeDef->isSensor; // センサーかどうかの設定
	impl->shape = b2CreatePolygonShape(box2dBodyPtr->GetBody(),  &def, box2dPolygonPtr->GetPolygon());
	box2dBodyPtr->SetShapeId(&impl->shape); // 形状IDをボディに設定
	pWorld->InsertShapeId(impl->shape); // ワールドに形状IDを追加
	isActive = true; // 有効フラグを設定
}

void physics::d2::box2dShape::Destroy()
{
	impl->pBody->RemoveShapeId(); // ボディから形状IDを削除
	impl->pBody = nullptr; // ポインタをnullptrに設定
	pWorld->RemoveShapeId(impl->shape); // ワールドから形状IDを削除
	b2DestroyShape(impl->shape, true); // 形状を削除
	isActive = false; // 有効フラグを無効に設定
}
