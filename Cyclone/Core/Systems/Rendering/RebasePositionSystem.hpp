#pragma once

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::Core::Systems::Rendering
{
	class RebasePositionSystem
	{
	public:
		RebasePositionSystem( entt::registry &inRegistry );

		void OnUpdateEnd( Cyclone::Core::LevelInterface *inLevelInterface );

	protected:
		entt::reactive_mixin<entt::storage<void>> mStorage;
	};
}