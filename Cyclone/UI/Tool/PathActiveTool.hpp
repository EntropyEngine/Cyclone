#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class PathActiveTool : public BaseTool
	{
	public:
		virtual const char *GetDebugName() const override { return "Path\nAct"; }
		virtual ECategory	GetCategory() const override { return ECategory::EditPath; }
		virtual ESelectMode	GetSelectMode() const override { return ESelectMode::ActiveInCategory; }

		virtual void		OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override;

	protected:
		template<EViewportType T>
		void				OnDraw( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
	};
}