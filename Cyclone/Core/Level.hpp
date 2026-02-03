#pragma once

namespace Cyclone
{
	namespace Core
	{
		class Level
		{
		public:
			Level() = default;

		protected:
			entt::registry mRegistry;
		};
	}
}