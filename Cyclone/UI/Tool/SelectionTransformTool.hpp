#pragma once

// Cyclone UI
#include "Cyclone/UI/ViewportType.hpp"
#include "Cyclone/UI/ViewportContext.hpp"

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
		template<EViewportType T>
		void OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext, ImDrawList *inDrawList, const ImVec2 &inViewOrigin, const ImVec2 &inSelectedBoxMin, const ImVec2 &inSelectedBoxMax );
	};
}