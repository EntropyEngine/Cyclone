#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core::Editor
{
	struct GridContext : Cyclone::Util::NonCopyable
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
}