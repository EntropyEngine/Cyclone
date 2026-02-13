#pragma once

// Cyclone Compontents
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// Cyclone Utils
#include "Cyclone/Util/Color.hpp"

namespace Cyclone::Core::Entity
{
	class PointDebug
	{
	public:
		static constexpr entt::hashed_string kEntityType = "point_debug"_hs;
		static constexpr entt::hashed_string kEntityCategory = "point"_hs;
		//static constexpr uint32_t kDebugColor = 0xffdd1800;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = inRegistry.create();

			// Attach a tagging components
			inRegistry.emplace<Cyclone::Core::Component::EntityType>( entity, static_cast<Component::EntityType>( kEntityType.value() ) );

			// Attach a Position component
			inRegistry.emplace<Cyclone::Core::Component::Position>( entity, inPosition );

			// Attach default center and extents (25cm radius)
			inRegistry.emplace<Cyclone::Core::Component::BoundingBox>( entity, Cyclone::Math::Vector4D::sZero(), Cyclone::Math::Vector4D::sReplicate( 0.25 ) );

			return entity;
		}
	};
}