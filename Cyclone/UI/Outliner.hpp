#pragma once

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class Outliner
	{
	public:
		void Update( Cyclone::Core::LevelInterface *inEntityInterface );
	};
}