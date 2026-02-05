#pragma once

namespace Cyclone::Core
{
	class Level
	{
	public:
		Level() = default;

		void					Initialize();

		const entt::registry &	GetRegistry() const { return mRegistry; }
		entt::registry &		GetRegistry()       { return mRegistry; }

	protected:
		entt::registry			mRegistry;
	};
}