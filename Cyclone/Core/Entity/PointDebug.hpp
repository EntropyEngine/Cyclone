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
	class PointDebug : public BaseEntity<PointDebug>
	{
	public:
		static constexpr entt::hashed_string kEntityType = "point_debug"_hs;
		static constexpr entt::hashed_string kEntityCategory = "point"_hs;
		//static constexpr uint32_t kDebugColor = 0xffdd1800;

		using history_components = entt::type_list_cat_t<BaseEntity::history_components, entt::type_list<>>;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = BaseEntity::sCreate( inRegistry );
			entt::handle handle = { inRegistry, entity };

			// Attach a Position component
			handle.emplace<Cyclone::Core::Component::Position>( inPosition );

			// Attach a Rotation component
			handle.emplace<Cyclone::Core::Component::Rotation>( DirectX::g_XMZero );

			// Attach default center and extents (25cm radius)
			handle.emplace<Cyclone::Core::Component::BoundingBox>( Cyclone::Math::Vector4D::sZero(), Cyclone::Math::Vector4D::sZero() );

			// Attach corresponding local bounds
			handle.emplace<Cyclone::Core::Component::LocalBounds>( DirectX::g_XMZero, DirectX::XMFLOAT3( 0.25, 0.25, 0.25 ), Cyclone::Core::Component::LocalBounds::EType::Radius ).UpdateBoundingBox( handle );

			return entity;
		}
	};
}