#pragma once

#include "Cyclone/Core/Component/EntityType.hpp"

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/Color.hpp"

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
			auto factory = entt::meta_factory<T>{}.type( T::kEntityType );
			factory = factory.data<&T::kEntityType>( "entity_type"_hs );
			factory = factory.data<&T::kEntityCategory>( "category"_hs );

			if constexpr ( requires { T::kDebugColor; } ) {
				factory = factory.data<&T::kDebugColor>( "debug_color"_hs );
			}
			else {
				switch ( T::kEntityCategory.value() ) {
					case "point"_hs.value():	factory = factory.data<Cyclone::Util::ColorU32( 0xDD, 0x18, 0xDD )>( "debug_color"_hs ); break;
					default:					factory = factory.data<Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF )>( "debug_color"_hs ); break;
				}
			}
		}
	};
}