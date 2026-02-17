#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core
{
	class Level : public Cyclone::Util::NonCopyable
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