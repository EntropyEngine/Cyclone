#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Component
{
	struct BoundingBox
	{
		Cyclone::Math::Vector4D mCenter;
		Cyclone::Math::Vector4D mExtent;
	};
}