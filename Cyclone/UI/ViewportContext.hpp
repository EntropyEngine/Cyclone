#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::UI
{
	struct ViewportGridContext : Cyclone::Util::NonCopyable
	{
		static constexpr double kGridSizes[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0 };
		static constexpr const char *kGridSizeText[] = { "1cm", "5cm", "10cm", "25cm", "50cm", "1m", "2.5m", "5m", "10m" };

		enum class ESnapType { ToGrid, ByGrid, None };

		ESnapType				mSnapType = ESnapType::ToGrid;
		int						mGridSizeIndex = 5;
		double					mGridSize = kGridSizes[mGridSizeIndex];
		double					mWorldLimit = 10000.0;

		void					SetGridSize( int inIndex )
		{
			mGridSizeIndex = std::clamp( inIndex, 0, static_cast<int>( std::size( kGridSizes ) ) - 1 );
			mGridSize = kGridSizes[mGridSizeIndex];
		}
	};

	struct ViewportPerspectiveContext : Cyclone::Util::NonCopyable
	{
		float					mCameraPitch = 0.0f;
		float					mCameraYaw = 0.0f;
		Cyclone::Math::Vector4D mCenter3D = Cyclone::Math::Vector4D( 3, 3, -6 );
	};

	struct ViewportOrthographicContext : Cyclone::Util::NonCopyable
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