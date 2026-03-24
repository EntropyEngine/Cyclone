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

		// void Activate() ????

		void Deactivate()
		{
			mCurrentAxis = ETransformAxis::None;
			mActiveEntity = entt::null;
			mInitialEntityPosition = Cyclone::Math::Vector4D::sZero();
			mInitialMousePosition = Cyclone::Math::Vector4D::sZero();
		}

	protected:
		// Current transform axis
		// Current entity
		// Entity original position
		// Mouse original position

		uint32_t				mCurrentAxis = ETransformAxis::None;
		entt::entity			mActiveEntity = entt::null;
		Cyclone::Math::Vector4D mInitialEntityPosition = Cyclone::Math::Vector4D::sZero();
		Cyclone::Math::Vector4D mInitialMousePosition = Cyclone::Math::Vector4D::sZero();

	};
}