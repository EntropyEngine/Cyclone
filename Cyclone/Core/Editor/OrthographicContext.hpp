#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core::Editor
{
	struct OrthographicContext : Cyclone::Util::NonCopyable
	{
		static double sZoomLevelToScale( int inLevel ) { return std::pow( 10.0, static_cast<double>( inLevel ) / 20.0 - 2.0 ); }

		int						mZoomLevel = 0;
		double					mZoomScale2D = sZoomLevelToScale( mZoomLevel ); // Pixels to meters
		Cyclone::Math::Vector4D mCenter2D = Cyclone::Math::Vector4D::sZero();

		void					UpdateZoomLevel( int inZoomLevel )
		{
			mZoomLevel = inZoomLevel;
			mZoomScale2D = sZoomLevelToScale( mZoomLevel );
		}
	};
}