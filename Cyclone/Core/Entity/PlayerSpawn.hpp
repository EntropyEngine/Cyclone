#pragma once

// Cyclone entites
#include "Cyclone/Core/Entity/BaseEntity.hpp"

// Cyclone Compontents
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

// Cyclone Utils
#include "Cyclone/Util/Color.hpp"

namespace Cyclone::Core::Entity
{
	class PlayerSpawn : public BaseEntity<PlayerSpawn>
	{
	public:
		static constexpr entt::hashed_string kEntityType = "player_spawn"_hs;
		static constexpr entt::hashed_string kEntityCategory = "player"_hs;

		using history_components = entt::type_list_cat_t<BaseEntity::history_components, entt::type_list<>>;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = BaseEntity::sCreate( inRegistry );

			// Attach a Position component
			inRegistry.emplace<Cyclone::Core::Component::Position>( entity, inPosition );

			// Attach a Rotation component
			inRegistry.emplace<Cyclone::Core::Component::Rotation>( entity, DirectX::g_XMZero );

			// Attach default center and extents (25cm radius)
			auto& box = inRegistry.emplace<Cyclone::Core::Component::BoundingBox>( entity, Cyclone::Math::Vector4D( 0.0, 0.5, 0.0 ), Cyclone::Math::Vector4D::sReplicate( 0.5 ) );

			// Attach corresponding local bounds
			inRegistry.emplace<Cyclone::Core::Component::LocalBounds>( entity, box.mValue.mCenter.ToXMVECTOR(), DirectX::XMFLOAT3( 0.5, 0.5, 0.5 ), Cyclone::Core::Component::LocalBounds::EType::Radius ).UpdateBoundingBox( entity, inRegistry );

			return entity;
		}
	};
}