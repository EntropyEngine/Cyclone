#pragma once

// Cyclone entites
#include "Cyclone/Core/Entity/BaseEntity.hpp"

// Cyclone Compontents
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

// Cyclone Utils
#include "Cyclone/Util/Color.hpp"

namespace Cyclone::Core::Entity
{
	class InfoDebug : public BaseEntity<InfoDebug>
	{
	public:
		static constexpr entt::hashed_string kEntityType = "info_debug"_hs;
		static constexpr entt::hashed_string kEntityCategory = "info"_hs;

		using history_components = entt::type_list_cat_t<BaseEntity::history_components, entt::type_list<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>>;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = BaseEntity::sCreate( inRegistry );

			// Attach a Position component
			inRegistry.emplace<Cyclone::Core::Component::Position>( entity, inPosition );

			// Attach default center and extents (25cm radius)
			inRegistry.emplace<Cyclone::Core::Component::BoundingBox>( entity, Cyclone::Math::Vector4D::sZero(), Cyclone::Math::Vector4D::sReplicate( 0.25 ) );

			return entity;
		}
	};
}