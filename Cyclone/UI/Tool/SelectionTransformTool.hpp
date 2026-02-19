#pragma once

// Cyclone UI
#include "Cyclone/UI/ViewportType.hpp"

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

struct ImDrawList;
struct ImVec2;

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
		void OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, ImDrawList *inDrawList, const ImVec2 &inViewOrigin, const ImVec2 &inSelectedBoxMin, const ImVec2 &inSelectedBoxMax );
	};
}