#ifndef D2_CONVERTER_H
#define D2_CONVERTER_H

#include "2D/d2_body.h"

// Box2D, ChoPhysics
#include <box2d.h>
#include <chomath.h>

namespace physics
{
	namespace d2
	{
		b2BodyType ConvertBodyType(Id2BodyType type) noexcept;
	}
}

#endif // D2_CONVERTER_H
