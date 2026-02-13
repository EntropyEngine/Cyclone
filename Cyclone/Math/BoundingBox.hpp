#pragma once

#include "Cyclone/Math/Vector.hpp"

#include "DirectXCollision.h"

namespace Cyclone::Math
{
	template<typename T>
	struct BoundingBox
	{
		using vector_type = T;
		using scalar_type = T::scalar_type;

		T mCenter;
		T mExtent;

		//BoundingBox( T inCenter, T inExtent ) : mCenter( inCenter ), mExtent( inExtent ) {}

		static BoundingBox XM_CALLCONV sFromMinMax( T inMin, T inMax )
		{
			const T half = T::sReplicate( static_cast<scalar_type>( 0.5 ) );

			T center = ( inMax + inMin ) * half;
			T extent = ( inMax - inMin ) * half;

			return BoundingBox( center, extent );
		}

		bool XM_CALLCONV Intersects( BoundingBox inRhs )
		{
			const T minA = mCenter - mExtent;
			const T maxA = mCenter + mExtent;

			const T minB = inRhs.mCenter - inRhs.mExtent;
			const T maxB = inRhs.mCenter + inRhs.mExtent;

			const auto disjoint = T::sBitwiseOr( T::sGreater( minA, maxB ), T::sGreater( minB, maxA ) );
			return !( T::sAnyTrue( disjoint ) );
		}
	};
}