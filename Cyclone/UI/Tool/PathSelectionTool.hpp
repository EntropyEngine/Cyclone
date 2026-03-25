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

		virtual void		OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *, const ViewportData & ) override {};
		virtual void		OnDraw( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData & ) override {}
		virtual void		OnRender( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData &, DrawType * ) override {}

	protected:
	};
}