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
#include "Cyclone/Core/Component/Mesh.hpp"

// Cyclone Utils
#include "Cyclone/Util/Color.hpp"

namespace Cyclone::Core::Entity
{
	class TerrainPathSpan : public BaseEntity<TerrainPathSpan>
	{
	public:
		static constexpr entt::hashed_string kEntityType = "terrain_path_span"_hs;
		static constexpr entt::hashed_string kEntityCategory = "terrain"_hs;

		using history_components = entt::type_list_cat_t<BaseEntity::history_components, entt::type_list<Component::MeshTag, Component::PathDependency>>;

		// TODO: add tag list and auto apply

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

			// Attach mesh tag
			handle.emplace<Component::MeshTag>();

			// Attach path dependency
			handle.emplace<Component::PathDependency>();

			return entity;
		}

		void OnDelete( entt::registry &inRegistry, entt::entity inEntity, std::set<entt::entity> &ioDirtyEntities )
		{
			entt::handle handle = { inRegistry, inEntity };
			auto &pathDependency = handle.get<Component::PathDependency>();

			if ( pathDependency.mPathEntity == entt::null ) return;

			auto view = inRegistry.view<Component::PathChildren>();
			assert( view.contains( pathDependency.mPathEntity ) );

			if ( view.get<Component::PathChildren>( pathDependency.mPathEntity ).RemoveChild( inEntity ) ) {
				ioDirtyEntities.insert( pathDependency.mPathEntity );
			}
		}
	};
}