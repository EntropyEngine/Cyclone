#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Math
{
	template<typename T>
	struct BoundingBox
	{
		using vector_type = T;

		T mCenter;
		T mExtent;
	};
}