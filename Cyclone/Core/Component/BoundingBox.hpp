#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Component
{
	struct BoundingBox
	{
		Cyclone::Math::XLVector mCenter;
		Cyclone::Math::XLVector mExtent;
	};
}