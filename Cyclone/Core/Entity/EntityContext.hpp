#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/Color.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"

namespace Cyclone::Core::Entity
{
	class EntityContext: public Cyclone::Util::NonCopyable
	{
	public:
		EntityContext() {}

		void Register();

		const char *GetEntityTypeName( Cyclone::Core::Component::EntityType inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityTypeNameMap.begin(), mEntityTypeNameMap.end(), hash );
			if ( it != mEntityTypeNameMap.end() && it->mKey == hash ) return it->mValue;
			return nullptr;
		}

		const char *GetEntityCategoryName( Cyclone::Core::Component::EntityCategory inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityCategoryNameMap.begin(), mEntityCategoryNameMap.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityCategoryNameMap.end() && it->mKey == hash ) return it->mValue;
			return nullptr;
		}

		uint32_t GetEntityTypeColor( Cyclone::Core::Component::EntityType inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityTypeColorMap.begin(), mEntityTypeColorMap.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityTypeColorMap.end() && it->mKey == hash ) return it->mValue;
			return Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF );
		}


		bool *GetEntityTypeIsSelectable( Cyclone::Core::Component::EntityType inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityTypeSelectable.begin(), mEntityTypeSelectable.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityTypeSelectable.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		const bool *GetEntityTypeIsSelectable( Cyclone::Core::Component::EntityType inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityTypeSelectable.begin(), mEntityTypeSelectable.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityTypeSelectable.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}


		bool *GetEntityTypeIsVisible( Cyclone::Core::Component::EntityType inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityTypeVisible.begin(), mEntityTypeVisible.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityTypeVisible.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		const bool *GetEntityTypeIsVisible( Cyclone::Core::Component::EntityType inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityTypeVisible.begin(), mEntityTypeVisible.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityTypeVisible.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}


		bool *GetEntityCategoryIsSelectable( Cyclone::Core::Component::EntityCategory inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityCategorySelectable.begin(), mEntityCategorySelectable.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityCategorySelectable.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		const bool *GetEntityCategoryIsSelectable( Cyclone::Core::Component::EntityCategory inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityCategorySelectable.begin(), mEntityCategorySelectable.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityCategorySelectable.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}


		bool *GetEntityCategoryIsVisible( Cyclone::Core::Component::EntityCategory inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityCategoryVisible.begin(), mEntityCategoryVisible.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityCategoryVisible.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		const bool *GetEntityCategoryIsVisible( Cyclone::Core::Component::EntityCategory inType ) const
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( mEntityCategoryVisible.begin(), mEntityCategoryVisible.end(), static_cast<entt::hashed_string::hash_type>( hash ) );
			if ( it != mEntityCategoryVisible.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

	protected:

		template<typename T>
		void RegisterEntityClass()
		{
			uint32_t entityColor = Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF );

			if constexpr ( requires { T::kDebugColor; } ) {
				entityColor = T::kDebugColor;
			}
			else {
				switch ( T::kEntityCategory.value() ) {
					case "ai"_hs.value():		entityColor = {}; break;
					case "camera"_hs.value():	entityColor = {}; break;
					case "env"_hs.value():		entityColor = {}; break;
					case "func"_hs.value():		entityColor = {}; break;
					case "game"_hs.value():		entityColor = {}; break;
					case "info"_hs.value():		entityColor = Cyclone::Util::ColorU32( 0x18, 0x18, 0xDD ); break;
					case "item"_hs.value():		entityColor = {}; break;
					case "light"_hs.value():	entityColor = {}; break;
					case "logic"_hs.value():	entityColor = {}; break;
					case "npc"_hs.value():		entityColor = {}; break;
					case "player"_hs.value():	entityColor = {}; break;
					case "point"_hs.value():	entityColor = Cyclone::Util::ColorU32( 0xDD, 0x18, 0xDD ); break;
					case "prop"_hs.value():		entityColor = {}; break;
					case "trigger"_hs.value():	entityColor = Cyclone::Util::ColorU32( 0xF8, 0x9A, 0x00 ); break;
				}
			}

			mEntityTypeColorMap.emplace_back( T::kEntityType.value(), entityColor );
			mEntityTypeNameMap.emplace_back( T::kEntityType.value(), T::kEntityType.data() );
			mEntityCategoryNameMap.emplace_back( T::kEntityCategory.value(), T::kEntityCategory.data() );
		}

		template<typename T>
		struct HashPair
		{
			entt::hashed_string::hash_type	mKey;
			T								mValue;
			bool operator < ( const HashPair &inRhs ) const { return mKey < inRhs.mKey; }
			bool operator ==( const HashPair &inRhs ) const { return mKey == inRhs.mKey; }
			operator entt::hashed_string::hash_type() const { return mKey; }
		};

		std::vector<HashPair<uint32_t>>		mEntityTypeColorMap;
		std::vector<HashPair<const char *>>	mEntityTypeNameMap;
		std::vector<HashPair<const char *>>	mEntityCategoryNameMap;

		std::vector<HashPair<bool>>			mEntityTypeSelectable;
		std::vector<HashPair<bool>>			mEntityTypeVisible;

		std::vector<HashPair<bool>>			mEntityCategorySelectable;
		std::vector<HashPair<bool>>			mEntityCategoryVisible;
	};
}