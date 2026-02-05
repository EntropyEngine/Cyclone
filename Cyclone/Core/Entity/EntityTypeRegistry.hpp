#pragma once

#include "Cyclone/Core/Component/EntityType.hpp"

#include "Cyclone/Core/Entity/PointDebug.hpp"

namespace Cyclone::Core::Entity
{
	constexpr entt::hashed_string gEntityTypeRegistry[] = {
		Cyclone::Core::Entity::PointDebug::kEntityType,
	};

	constexpr const char *GetEntityTypeName( Cyclone::Core::Component::EntityType inEntityType )
	{
		for ( size_t i = 0; i < std::size( gEntityTypeRegistry ); ++i ) {
			if ( static_cast<entt::hashed_string::hash_type>( inEntityType ) == gEntityTypeRegistry[i].value() ) {
				return gEntityTypeRegistry[i].data();
			}
		}
		return nullptr;
	}
}