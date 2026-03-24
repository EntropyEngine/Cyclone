#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class SelectionTransformTool : public BaseTool
	{
	public:
		virtual const char *GetDebugName() const override { return "Box"; }
		virtual void OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override;
		virtual void OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override;
		virtual void OnRender( EViewportType, Cyclone::Core::LevelInterface *, const ViewportData &, DrawType * ) override {}

	protected:
		template<EViewportType T>
		void OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );

		template<EViewportType T>
		void OnDraw( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
	};
}