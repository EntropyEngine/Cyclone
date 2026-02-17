#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class Outliner : public Cyclone::Util::NonCopyable
	{
	public:
		void Update( Cyclone::Core::LevelInterface *inLevelInterface );
	};
}