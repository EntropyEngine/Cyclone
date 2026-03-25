#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class MeshSelectionTool : public BaseTool
	{
	public:
		virtual const char *GetDebugName() const override { return "Mesh\nSel"; }
		virtual ECategory	GetCategory() const override { return ECategory::EditMesh; }

		virtual void		OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *, const ViewportData & ) override {};
		virtual void		OnDraw( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData & ) override {}
		virtual void		OnRender( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData &, DrawType * ) override {}

	protected:
	};
}