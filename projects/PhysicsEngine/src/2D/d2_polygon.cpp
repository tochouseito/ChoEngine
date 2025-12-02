#include "physicsengine_pch.h"
#include "2D/d2_polygon.h"
// box2d
#include <box2d.h>

using namespace physics::d2;

struct box2dPolygon::Impl
{
	b2Polygon polygon; // Box2Dのポリゴン
};

physics::d2::box2dPolygon::box2dPolygon()
	: impl(std::make_unique<box2dPolygon::Impl>())
{
}

void physics::d2::box2dPolygon::MakeBox(const float& halfWidth, const float& halfHeight)
{
	impl->polygon = b2MakeBox(halfWidth, halfHeight);
}

const b2Polygon* physics::d2::box2dPolygon::GetPolygon() const { return &impl->polygon; }

struct choPhysicsPolygon::Impl
{
	int dummy = 0;
};

physics::d2::choPhysicsPolygon::choPhysicsPolygon()
	: impl(std::make_unique<choPhysicsPolygon::Impl>())
{
}
