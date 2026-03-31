#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Component
{
	using PathTag = entt::tag<"path_tag"_hs>;

	struct PathData
	{
		PathData() = default;

		struct Segment
		{
			Cyclone::Math::Vector4D mControlPoints[4];
		};

		std::vector<Segment> mPathSegments;
	};
}