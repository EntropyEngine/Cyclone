#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Component
{
	struct alignas( 32 ) Position
	{
		Cyclone::Math::Vector4D mValue;
	};

	struct alignas( 16 ) RebasedPositionPerspective
	{
		DirectX::XMVECTOR mValue;
	};

	struct alignas( 16 ) RebasedPositionOrthographic
	{
		DirectX::XMVECTOR mValue;
	};
}