#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Component
{
	struct alignas(32) Position : public Cyclone::Math::Vector4D {};
}