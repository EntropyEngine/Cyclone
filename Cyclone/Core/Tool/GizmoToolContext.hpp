#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::UI::Tool {
	class GizmoTransformTool;
}

namespace Cyclone::Core::Tool
{
	class GizmoToolContext : public Cyclone::Util::NonCopyable
	{
	public:
		friend Cyclone::UI::Tool::GizmoTransformTool;

		enum ETransformAxis : uint32_t
		{
			None = 0,
			XAxis = 1 << 0,
			YAxis = 1 << 1,
			ZAxis = 1 << 2,
		};

		enum class ETransformType
		{
			Translate,
			Rotate
		};

		// void Activate() ????

		void Deactivate()
		{
			mCurrentAxis = ETransformAxis::None;
			mActiveEntity = entt::null;
			mInitialEntityPosition = Cyclone::Math::Vector4D::sZero();
			mInitialMousePosition = Cyclone::Math::Vector4D::sZero();
		}

		void GetSingleAxis( Cyclone::Math::Vector4D &outAxis ) const
		{
			outAxis = Cyclone::Math::Vector4D(
				( mCurrentAxis & GizmoToolContext::XAxis ) ? 1.0 : 0.0,
				( mCurrentAxis & GizmoToolContext::YAxis ) ? 1.0 : 0.0,
				( mCurrentAxis & GizmoToolContext::ZAxis ) ? 1.0 : 0.0
			);
		}

		void GetDualAxis( Cyclone::Math::Vector4D &outAxis1, Cyclone::Math::Vector4D &outAxis2 ) const
		{
			bool first = true;
			for ( int i = 0; i < 3; ++i ) {
				if ( mCurrentAxis & ( 1 << i ) ) {
					( first ? outAxis1 : outAxis2 ) = Cyclone::Math::Vector4D::sZeroSetValueByIndex( i, 1.0 );
					first = false;
				}
			}
		}

	protected:
		// Current transform axis
		// Current entity
		// Entity original position
		// Mouse original position

		ETransformType			mTransformType = ETransformType::Rotate;
		uint32_t				mCurrentAxis = ETransformAxis::None;
		entt::entity			mActiveEntity = entt::null;
		Cyclone::Math::Vector4D mInitialEntityPosition = Cyclone::Math::Vector4D::sZero();
		Cyclone::Math::Vector4D mInitialMousePosition = Cyclone::Math::Vector4D::sZero();
		DirectX::XMVECTOR		mInitialEntityRotation = DirectX::g_XMZero;

	};
}