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
	class SelectionTransformTool : public Cyclone::Util::NonCopyable
	{
	public:
		SelectionTransformTool() = default;

		template<EViewportType T>
		void OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface );

		template<EViewportType T>
		void OnDraw( Cyclone::Core::LevelInterface *inLevelInterface );
	};
}