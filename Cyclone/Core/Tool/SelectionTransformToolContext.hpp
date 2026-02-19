#pragma once

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

	protected:
		Cyclone::Math::Vector4D mInitialPosition = Cyclone::Math::Vector4D::sZero();
		entt::entity mActiveEntity = entt::null;
	};
}