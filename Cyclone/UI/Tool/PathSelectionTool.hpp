#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class PathSelectionTool : public BaseTool
	{
	public:
		virtual const char *GetDebugName() const override { return "Path\nSel"; }
		virtual ECategory	GetCategory() const override { return ECategory::EditPath; }
		virtual ESelectMode	GetSelectMode() const override { return ESelectMode::SelectInCategory; }

	protected:
	};
}