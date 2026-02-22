#pragma once

#include "Cyclone/Math/BoundingBox.hpp"

namespace Cyclone::Core::Component
{
	struct BoundingBox
	{
		Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> mValue;
	};
}