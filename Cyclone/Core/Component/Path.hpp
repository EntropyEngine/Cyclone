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
			Cyclone::Math::Vector4D mP0;
			Cyclone::Math::Vector4D mP1;
			Cyclone::Math::Vector4D mP2;
			Cyclone::Math::Vector4D mP3;

			Cyclone::Math::Vector4D XM_CALLCONV GetPoint( double u ) const
			{
				const double u2 = u * u;
				const double u3 = u2 * u;

				const double iu = 1.0 - u;
				const double iu2 = iu * iu;
				const double iu3 = iu2 * iu;

				const Cyclone::Math::Vector4D b0 = Cyclone::Math::Vector4D::sReplicate( iu3 );
				const Cyclone::Math::Vector4D b1 = Cyclone::Math::Vector4D::sReplicate( 3.0 * u * iu2 );
				const Cyclone::Math::Vector4D b2 = Cyclone::Math::Vector4D::sReplicate( 3.0 * u2 * iu );
				const Cyclone::Math::Vector4D b3 = Cyclone::Math::Vector4D::sReplicate( u3 );

				return mP0 * b0 + mP1 * b1 + mP2 * b2 + mP3 * b3;
			}
		};

		std::vector<Segment> mPathSegments;
	};
}