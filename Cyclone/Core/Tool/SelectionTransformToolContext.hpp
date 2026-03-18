#pragma once

// Cyclone Components
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::UI::Tool {
	class SelectionTransformTool;
}

namespace Cyclone::Core::Tool
{
	class SelectionTransformToolContext : public Cyclone::Util::NonCopyable
	{
	public:
		SelectionTransformToolContext() = default;

		friend Cyclone::UI::Tool::SelectionTransformTool;

		entt::entity GetActiveEntity() const { return mActiveEntity; }
		bool IsActiveEntity( entt::entity inEntity ) const { return inEntity == mActiveEntity; }
		void XM_CALLCONV SetActiveEntity( entt::entity inEntity, Cyclone::Math::Vector4D inPosition ) { mActiveEntity = inEntity; mInitialPosition = inPosition; }
		Cyclone::Math::Vector4D XM_CALLCONV	GetInitialPosition() const { return mInitialPosition; }
		void Deactivate() { mActiveEntity = entt::null; }

		void OnPreUpdate()
		{
			mSelectionMin = Cyclone::Math::Vector4D::sPosInf();
			mSelectionMax = Cyclone::Math::Vector4D::sNegInf();
		}

		// TODO: const or const&?
		void XM_CALLCONV IncludeSelectedEntity( const Cyclone::Core::Component::Position &inPosition, const Cyclone::Core::Component::BoundingBox &inBounds )
		{
			Cyclone::Math::Vector4D center = inPosition.mValue + inBounds.mValue.mCenter;
			Cyclone::Math::Vector4D boxmax = center + inBounds.mValue.mExtent;
			Cyclone::Math::Vector4D boxmin = center - inBounds.mValue.mExtent;

			mSelectionMin = Cyclone::Math::Vector4D::sMin( mSelectionMin, boxmin );
			mSelectionMax = Cyclone::Math::Vector4D::sMax( mSelectionMax, boxmax );
		}

		void XM_CALLCONV UpdateOnDrag( const Cyclone::Math::Vector4D inDelta )
		{
			mSelectionMin += inDelta;
			mSelectionMax += inDelta;
		}

	protected:
		Cyclone::Math::Vector4D mSelectionMin = Cyclone::Math::Vector4D::sPosInf();
		Cyclone::Math::Vector4D mSelectionMax = Cyclone::Math::Vector4D::sNegInf();
		Cyclone::Math::Vector4D mInitialPosition = Cyclone::Math::Vector4D::sZero();
		entt::entity mActiveEntity = entt::null;
	};
}