#include "physicsengine_pch.h"
#include "2D/d2_converter.h"

using namespace physics::d2;

b2BodyType physics::d2::ConvertBodyType(Id2BodyType type) noexcept
{
	switch (type)
	{
	case Id2BodyType::Id2_staticBody:
		return b2_staticBody;
	case Id2BodyType::Id2_kinematicBody:
		return b2_kinematicBody;
	case Id2BodyType::Id2_dynamicBody:
		return b2_dynamicBody;
	default:
		return b2_staticBody; // デフォルトは静的ボディ
	}
}
