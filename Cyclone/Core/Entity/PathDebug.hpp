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

		using history_components = entt::type_list_cat_t<BaseEntity::history_components, entt::type_list<Component::PathTag, Component::PathData, Component::PathSelection>>;

		entt::entity Create( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			// Allocates in the entity storage of the registry
			entt::entity entity = BaseEntity::sCreate( inRegistry );
			entt::handle handle = { inRegistry, entity };

			// Attach a Position component
			handle.emplace<Cyclone::Core::Component::Position>( inPosition );

			// Attach a Rotation component
			handle.emplace<Cyclone::Core::Component::Rotation>( DirectX::g_XMZero );

			// Attach empty BB and local bounds
			handle.emplace<Cyclone::Core::Component::BoundingBox>( Cyclone::Math::Vector4D::sZero(), Cyclone::Math::Vector4D::sZero() );
			Component::LocalBounds &localBounds = handle.emplace<Cyclone::Core::Component::LocalBounds>( DirectX::g_XMZero, DirectX::XMFLOAT3(), Cyclone::Core::Component::LocalBounds::EType::Path );

			// Attach path tag and data
			handle.emplace<Component::PathSelection>();
			handle.emplace<Component::PathTag>();
			Component::PathData &pathData = handle.emplace<Component::PathData>();

			pathData.AddKnot();
			pathData.AddKnot();
			pathData.AddKnot();
			pathData.AddFullLoop( DirectX::XM_2PI, 5, 3.5 );
			pathData.AddKnot();
			pathData.AddCurve( DirectX::XM_PIDIV2 * 1.0 / 2.0 );

			pathData.AddCurve( DirectX::XM_PIDIV2 * 1.0 / 2.0 );
			pathData.mPathWidths.back() = 6.0f;
			pathData.mExtrusionTypes.back() &= ~Component::PathData::EExtrusionType::CurveBitangent;

			pathData.AddKnot();
			pathData.mPathWidths.back() = 6.0f;

			pathData.AddKnot();
			pathData.AddKnot();
			pathData.mPathWidths.back() = 6.0f;

			pathData.AddKnot();
			pathData.mPathWidths.back() = 6.0f;

			pathData.AddKnot();
			pathData.mExtrusionTypes.back() &= ~Component::PathData::EExtrusionType::EaseIn;
			pathData.mPathWidths.back() = 2.0f;

			//pathData.mKnots[pathData.mKnots.size() - 3].mPoint += Cyclone::Math::Vector4D( 0.0, 4.0, 0.0 );

			pathData.ValidatePath();

			localBounds.UpdateBoundingBox( handle );

			return entity;
		}

		void SynchroniseAuxiliaryComponents( entt::registry &inRegistry, entt::entity inEntity )
		{
			entt::handle handle = { inRegistry, inEntity };
			handle.get_or_emplace<Component::PathCache>().Rebuild( handle );
			handle.get_or_emplace<Component::PathChildren>().FindChildren( handle );
		}

		void SynchroniseChildren( entt::registry &inRegistry, entt::entity inEntity )
		{

		}

		void OnDelete( entt::registry &inRegistry, entt::entity inEntity, std::set<entt::entity> &ioDirtyEntities )
		{
			entt::handle handle = { inRegistry, inEntity };
			auto &pathChildren = handle.get<Component::PathChildren>();

			auto view = inRegistry.view<Component::PathDependency>();

			for ( entt::entity child : pathChildren.mChildren ) {
				assert( view.contains( child ) );

				auto &pathDependency = view.get<Component::PathDependency>( child );

				assert( pathDependency.mPathEntity == inEntity );

				if ( pathDependency.Reset() ) {
					ioDirtyEntities.insert( child );
				}
			}
		}
	};
}