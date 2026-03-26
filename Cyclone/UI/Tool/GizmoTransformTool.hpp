#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

// Cyclone Math
#include "Cyclone/Math/Vector.hpp"

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

		struct				TranslateInput
		{
			Cyclone::Math::Vector4D mOriginalPos{ nullptr };
			Cyclone::Math::Vector4D mCameraPos{ nullptr };
			Cyclone::Math::Vector4D mMousePosNear{ nullptr };
			Cyclone::Math::Vector4D mMousePosFar{ nullptr };
		};

		struct				TranslateOutput
		{
			Cyclone::Math::Vector4D mMousePos{ nullptr };
			Cyclone::Math::Vector4D mAxisMask{ nullptr };
			Cyclone::Math::Vector4D mAxisMaskInv{ nullptr };
		};

	protected:
		template<EViewportType T>
		void				OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );

		void				OnUpdatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
		void				OnRenderPerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch );
	};
}