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

			// Attach empty BB and local bounds
			inRegistry.emplace<Cyclone::Core::Component::BoundingBox>( entity, Cyclone::Math::Vector4D::sZero(), Cyclone::Math::Vector4D::sZero() );
			Component::LocalBounds &localBounds = inRegistry.emplace<Cyclone::Core::Component::LocalBounds>( entity, DirectX::g_XMZero, DirectX::XMFLOAT3(), Cyclone::Core::Component::LocalBounds::EType::Path );

			// Attach path tag and data
			inRegistry.emplace<Component::PathTag>( entity );
			Component::PathData &pathData = inRegistry.emplace<Component::PathData>( entity );

			pathData.mPathSegments.emplace_back(
				Cyclone::Math::Vector4D( 0.0, 0.0, 0.0 ),
				Cyclone::Math::Vector4D( 0.0, 0.0, 1.0 ),
				Cyclone::Math::Vector4D( 1.0, 1.0, 2.0 ),
				Cyclone::Math::Vector4D( 1.0, 1.0, 3.0 )
			);

			localBounds.UpdateBoundingBox( entity, inRegistry );

			return entity;
		}
	};
}