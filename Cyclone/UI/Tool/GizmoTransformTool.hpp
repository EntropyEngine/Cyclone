#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class GizmoTransformTool : public BaseTool
	{
	public:
		virtual const char *GetDebugName() const override { return "Gizmo"; }
		virtual ECategory	GetCategory() const override { return ECategory::Object; }

		virtual void		OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override;
		virtual void		OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) override {}
		virtual void		OnRender( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch ) override;

	protected:
		template<EViewportType T>
		void				OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );

		template<EViewportType T>
		void				OnDraw( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );

		void				OnUpdatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
		void				OnRenderPerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch );
	};
}