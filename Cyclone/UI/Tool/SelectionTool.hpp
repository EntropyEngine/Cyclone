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
		virtual ECategory	GetCategory() const override { return ECategory::Object; }
		virtual ESelectMode	GetSelectMode() const override { return ESelectMode::ToggleInCategory; }

		virtual void		OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override;

	protected:
		template<EViewportType T>
		void				OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );

		void				OnUpdatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
	};
}