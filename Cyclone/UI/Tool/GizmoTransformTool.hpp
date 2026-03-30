#pragma once

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

// Cyclone Math
#include "Cyclone/Math/Vector.hpp"

// STL
#include <bit>

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
		void				UpdateTranslateOrthographic( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
		void				UpdateTranslatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
		void				UpdateTranslate( Cyclone::Core::LevelInterface *inLevelInterface, const TranslateInput &inInput, const TranslateOutput &inOutput );

		template<EViewportType T>
		void				UpdateRotateOrthographic( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
		void				UpdateRotatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData );
		void XM_CALLCONV	UpdateRotate( Cyclone::Core::LevelInterface *inLevelInterface, DirectX::FXMMATRIX modelMatrix );

		template<EViewportType T>
		int					GetTypedID( entt::entity inEntity, int prefix )
		{
			static constexpr int shift = std::popcount( entt::entt_traits<entt::entity>::entity_mask );
			return ( static_cast<int>( T ) << shift ) + ( prefix << ( shift + 2 ) ) + static_cast<int>( inEntity );
		}
	};
}