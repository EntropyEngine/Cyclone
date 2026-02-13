#pragma once

#include "Cyclone/Math/BoundingBox.hpp"

namespace Cyclone::Core::Component
{
	struct BoundingBox : public Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> {};
}