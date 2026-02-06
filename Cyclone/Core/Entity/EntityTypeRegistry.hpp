#pragma once

#include "Cyclone/Core/Entity/PointDebug.hpp"

#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core::Entity
{
	class EntityTypeRegistry : public Cyclone::Util::NonCopyable
	{
	public:
		static EntityTypeRegistry sInstance;

		static const char *GetEntityTypeName( Cyclone::Core::Component::EntityType inEntityType )
		{
			const auto &it = sInstance.mEntityTypes.find( static_cast<entt::hashed_string::hash_type>( inEntityType ) );
			if ( it != sInstance.mEntityTypes.end() ) {
				return it->second;
			}
			return nullptr;
		}

		static const char *GetEntityCategoryName( Cyclone::Core::Component::EntityCategory inEntityCategory )
		{
			const auto &it = sInstance.mEntityCategories.find( static_cast<entt::hashed_string::hash_type>( inEntityCategory ) );
			if ( it != sInstance.mEntityCategories.end() ) {
				return it->second;
			}
			return nullptr;
		}

	protected:
		EntityTypeRegistry();

		std::map<entt::hashed_string::hash_type, const char *> mEntityTypes;
		std::map<entt::hashed_string::hash_type, const char *> mEntityCategories;

		std::map<entt::hashed_string::hash_type, entt::hashed_string::hash_type> mEntityTypeToCategory;

		template<typename T>
		void RegisterEntityClass()
		{
			mEntityTypes.emplace( T::kEntityType.value(), T::kEntityType.data() );
			mEntityCategories.emplace( T::kEntityCategory.value(), T::kEntityCategory.data() );
			mEntityTypeToCategory.emplace( T::kEntityType.value(), T::kEntityCategory.value() );
		}
	};
}