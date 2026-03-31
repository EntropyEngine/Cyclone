#pragma once

// Cyclone entites
#include "Cyclone/Core/Entity/BaseEntity.hpp"

// Cyclone Compontents
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone Utils
#include "Cyclone/Util/Color.hpp"

namespace Cyclone::Core::Entity
{
	class PathDebug : public BaseEntity<PathDebug>
	{
	public:
		static constexpr entt::hashed_string kEntityType = "path_debug"_hs;
		static constexpr entt::hashed_string kEntityCategory = "path"_hs;

		using history_components = entt::type_list_cat_t<BaseEntity::history_components, entt::type_list<Component::PathTag, Component::PathData>>;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = BaseEntity::sCreate( inRegistry );

			// Attach a Position component
			inRegistry.emplace<Cyclone::Core::Component::Position>( entity, inPosition );

			// Attach a Rotation component
			inRegistry.emplace<Cyclone::Core::Component::Rotation>( entity, DirectX::g_XMZero );

			// Attach default center and extents (25cm radius)
			inRegistry.emplace<Cyclone::Core::Component::BoundingBox>( entity, Cyclone::Math::Vector4D::sZero(), Cyclone::Math::Vector4D::sZero() );

			// Attach corresponding local bounds
			inRegistry.emplace<Cyclone::Core::Component::LocalBounds>( entity, DirectX::g_XMZero, DirectX::XMFLOAT3( 0.25, 0.25, 0.25 ), Cyclone::Core::Component::LocalBounds::EType::Radius ).UpdateBoundingBox( entity, inRegistry );

			// Attach path tag and data
			inRegistry.emplace<Component::PathTag>( entity );
			inRegistry.emplace<Component::PathData>( entity );

			return entity;
		}
	};
}