#pragma once

#include "Cyclone/Core/Component/Position.hpp"

namespace Cyclone::Core::Entity
{
	class PointDebug
	{
	public:
		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::XLVector inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = inRegistry.create();

			// Attach a Position component
			inRegistry.emplace<Cyclone::Core::Component::Position>( entity, inPosition );

			return entity;
		}
	};
}