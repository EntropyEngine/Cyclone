#pragma once

// Cyclone UI
#include "Cyclone/UI/ViewportType.hpp"

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class SelectionTool : public Cyclone::Util::NonCopyable
	{
	public:
		SelectionTool() {}

		template<EViewportType T>
		void OnClick( Cyclone::Core::LevelInterface *inLevelInterface, double inWorldSpaceU, double inWorldSpaceV, double inHandleRadius, double inWorldLimit );
	};
}