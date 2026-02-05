#pragma once

namespace Cyclone::Core
{
	/// @brief The management interface that is used to add, modify or delete entities from a Level based on interactions within the user interface.
	///
	/// This class marshals data to and from the Level object's ECS into a form that the user can manipulate within the UI.
	/// This class also manages the "cursor" used to select entities or spawn them at specific coordinates with specific defaults.
	/// Finally this class has a registry of available entity types and manages their requirements.
	class EntityInterface
	{
	public:
		EntityInterface() = default;

		void Initialize();
	};
}