#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Component
{
	struct alignas( 16 ) Rotation
	{
		DirectX::XMVECTOR mPitchYawRoll;
	};
}