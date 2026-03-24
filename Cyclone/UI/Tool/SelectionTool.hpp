#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class SelectionTool : public BaseTool
	{
	public:
		virtual const char *GetDebugName() const override { return "Sel"; }
		virtual void OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override;
		virtual void OnDraw( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData & ) override {}
		virtual void OnRender( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData &, DrawType * ) override {}

	protected:
		template<EViewportType T>
		void OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
	};
}