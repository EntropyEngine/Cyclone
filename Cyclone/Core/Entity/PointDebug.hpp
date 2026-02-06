#pragma once

#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"

namespace Cyclone::Core::Entity
{
	class PointDebug
	{
	public:
		static constexpr entt::hashed_string kEntityType = "point_debug"_hs;
		static constexpr entt::hashed_string kEntityCategory = "point"_hs;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::XLVector inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = inRegistry.create();

			// Attach a tagging components
			inRegistry.emplace<Cyclone::Core::Component::EntityType>( entity, static_cast<Cyclone::Core::Component::EntityType>( kEntityType.value() ) );
			inRegistry.emplace<Cyclone::Core::Component::EntityCategory>( entity, static_cast<Cyclone::Core::Component::EntityCategory>( kEntityCategory.value() ) );

			// Attach a Position component
			inRegistry.emplace<Cyclone::Core::Component::Position>( entity, inPosition );

			return entity;
		}
	};
}