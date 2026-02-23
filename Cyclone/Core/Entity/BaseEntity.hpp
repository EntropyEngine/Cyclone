#pragma once

// Cyclone Compontents
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

template<typename T>
struct CopyComponentFunctor
{
	static void sApply( const entt::registry &inRegistry, entt::registry &inHistoryRegistry, entt::entity inEntity )
	{
		const T copy = inRegistry.get<T>( inEntity );
		inHistoryRegistry.emplace_or_replace<T>( inEntity, copy );
	}
};

template<typename List, size_t index = 0>
void ApplyTest( const entt::registry &inRegistry, entt::registry &inHistoryRegistry, entt::entity inEntity )
{
	if constexpr ( index < List::size ) {
		CopyComponentFunctor<entt::type_list_element_t<index, List>>::sApply( inRegistry, inHistoryRegistry, inEntity );
		ApplyTest<List, index + 1>( inRegistry, inHistoryRegistry, inEntity );
	}
}

namespace Cyclone::Core::Entity
{
	template<typename T>
	class BaseEntity
	{
	public:
		using history_components = entt::type_list<Component::EntityType, Component::EntityCategory, Component::Visible, Component::Selectable>;

		static void sRegister( entt::meta_ctx &inMetaContext )
		{
			static_assert( T::history_components::size > 4 );
			static_assert( entt::type_list_diff_t<T::history_components, history_components>::size + history_components::size == T::history_components::size );

			entt::meta_factory<T>{ inMetaContext }.type( T::kEntityType ).func<&T::sCreateEntity>( "create_entity"_hs );
		}

		static entt::entity sCreateEntity( entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
		{
			auto entity = T().Create( inRegistry, inPosition );
			assert( [&] <typename... Types>( entt::type_list<Types...> ) { return inRegistry.template all_of<Types...>( entity ); }( typename T::history_components{} ) );
			return entity;
		}

		static void sSaveHistory( const entt::registry &inRegistry, entt::registry &inHistoryRegistry, entt::entity inEntity )
		{
			if ( !inHistoryRegistry.valid( inEntity ) ) {
				auto retEntity = inHistoryRegistry.create( inEntity );
				assert( retEntity == inEntity );
			}
			ApplyTest<T::history_components>( inRegistry, inHistoryRegistry, inEntity );
		}

		static void sRestorHistory( entt::registry &inRegistry, const entt::registry &inHistoryRegistry, entt::entity inEntity )
		{

		}

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