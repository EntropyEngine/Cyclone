#pragma once

#include "Cyclone/Core/Component/EntityType.hpp"

#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core::Entity
{
	class EntityTypeRegistry : public Cyclone::Util::NonCopyable
	{
	public:
		using EntityType = Cyclone::Core::Component::EntityType;

		static EntityTypeRegistry sInstance;

		static const char *GetEntityTypeName( EntityType inEntityType )
		{
			return entt::resolve( static_cast<entt::id_type>( inEntityType ) ).data( "entity_type"_hs ).get( {} ).cast<entt::hashed_string>().data();
		}

		static entt::hashed_string GetEntityTypeCategory( EntityType inEntityType )
		{
			return entt::resolve( static_cast<entt::id_type>( inEntityType ) ).data( "category"_hs ).get( {} ).cast<entt::hashed_string>();
		}

		static const char *GetEntityTypeCategoryName( EntityType inEntityType )
		{
			return entt::resolve( static_cast<entt::id_type>( inEntityType ) ).data( "category"_hs ).get( {} ).cast<entt::hashed_string>().data();
		}

	protected:
		EntityTypeRegistry();

		template<typename T>
		void RegisterEntityClass()
		{
			entt::meta_factory<T>{}.type( T::kEntityType )
				.data<&T::kEntityType>( "entity_type"_hs )
				.data<&T::kEntityCategory>( "category"_hs );

			if constexpr ( requires { T::kDebugColor; } ) {
				entt::meta_factory<T>{}.type( T::kEntityType ).data<&T::kDebugColor>( "debug_color"_hs );
			}
			else {
				switch ( T::kEntityCategory.value() ) {
					case "point"_hs.value():	entt::meta_factory<T>{}.type( T::kEntityType ).data<0xffdd18dd>( "debug_color"_hs ); break;
					default:					entt::meta_factory<T>{}.type( T::kEntityType ).data<0xffffffff>( "debug_color"_hs ); break;
				}
			}
		}
	};
}