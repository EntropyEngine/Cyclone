#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core::Editor
{
	struct PerspectiveContext : Cyclone::Util::NonCopyable
	{
		float					mCameraPitch = 0.0f;
		float					mCameraYaw = 0.0f;
		Cyclone::Math::Vector4D mCenter3D = Cyclone::Math::Vector4D( 3, 3, -6 );
	};
}