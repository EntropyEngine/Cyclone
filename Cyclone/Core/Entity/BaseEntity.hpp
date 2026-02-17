#pragma once

// Cyclone Compontents
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

namespace Cyclone::Core::Entity
{
	template<typename T>
	class BaseEntity
	{
	protected:
		static entt::entity sCreate( entt::registry &inRegistry )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = inRegistry.create();

			// Attach a tagging components
			inRegistry.emplace<Cyclone::Core::Component::EntityType>( entity, static_cast<Component::EntityType>( T::kEntityType.value() ) );
			inRegistry.emplace<Cyclone::Core::Component::EntityCategory>( entity, static_cast<Component::EntityCategory>( T::kEntityCategory.value() ) );
			inRegistry.emplace<Cyclone::Core::Component::Visible>( entity, static_cast<Cyclone::Core::Component::Visible>( true ) );
			inRegistry.emplace<Cyclone::Core::Component::Selectable>( entity, static_cast<Cyclone::Core::Component::Selectable>( true ) );

			return entity;
		}
	};
}