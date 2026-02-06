#pragma once

#include "Cyclone/Core/Entity/PointDebug.hpp"

#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::Core::Entity
{
	class EntityTypeRegistry : public Cyclone::Util::NonCopyable
	{
	public:
		using EntityType = Cyclone::Core::Component::EntityType;
		using EntityCategory = Cyclone::Core::Component::EntityCategory;

		static EntityTypeRegistry sInstance;

		static const char *GetEntityTypeName( EntityType inEntityType )
		{
			const auto &it = sInstance.mEntityTypes.find( inEntityType );
			if ( it != sInstance.mEntityTypes.end() ) {
				return it->second;
			}
			return nullptr;
		}

		static const char *GetEntityCategoryName( EntityCategory inEntityCategory )
		{
			const auto &it = sInstance.mEntityCategories.find( inEntityCategory );
			if ( it != sInstance.mEntityCategories.end() ) {
				return it->second;
			}
			return nullptr;
		}

	protected:
		EntityTypeRegistry();

		std::map<EntityType, const char *> mEntityTypes;
		std::map<EntityCategory, const char *> mEntityCategories;

		std::map<EntityType, EntityCategory> mEntityTypeToCategory;

		template<typename T>
		void RegisterEntityClass()
		{
			constexpr auto entityType = static_cast<EntityType>( T::kEntityType.value() );
			constexpr auto entityCategory = static_cast<EntityCategory>( T::kEntityCategory.value() );

			mEntityTypes.emplace( entityType, T::kEntityType.data() );
			mEntityCategories.emplace( entityCategory, T::kEntityCategory.data() );
			mEntityTypeToCategory.emplace( entityType, entityCategory );
		}
	};
}