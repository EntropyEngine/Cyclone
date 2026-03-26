#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class ObjectProperties : public Cyclone::Util::NonCopyable
	{
	public:
		void ShowWindow( Cyclone::Core::LevelInterface *inLevelInterface, entt::entity inEntity );
	};
}