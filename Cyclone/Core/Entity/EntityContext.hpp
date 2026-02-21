#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/Color.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core::Entity
{
	class EntityContext: public Cyclone::Util::NonCopyable
	{
	public:
		EntityContext() {}

		void Register();


		const char *			GetEntityTypeName( Cyclone::Core::Component::EntityType inType ) const					{ auto it = sFindIn( mEntityTypeNameMap, inType ); return it ? *it : nullptr; }
		const char *			GetEntityCategoryName( Cyclone::Core::Component::EntityCategory inType ) const			{ auto it = sFindIn( mEntityCategoryNameMap, inType ); return it ? *it : nullptr; }
		uint32_t				GetEntityTypeColor( Cyclone::Core::Component::EntityType inType ) const					{ auto it = sFindIn( mEntityTypeColorMap, inType ); return it ? *it : Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF ); }

		bool *					GetEntityTypeIsSelectable( Cyclone::Core::Component::EntityType inType )				{ auto it = sFindIn( mEntityTypeSelectable, inType ); return it ? it : nullptr; }
		const bool *			GetEntityTypeIsSelectable( Cyclone::Core::Component::EntityType inType ) const			{ auto it = sFindIn( mEntityTypeSelectable, inType ); return it ? it : nullptr; }

		bool *					GetEntityTypeIsVisible( Cyclone::Core::Component::EntityType inType )					{ auto it = sFindIn( mEntityTypeVisible, inType ); return it ? it : nullptr; }
		const bool *			GetEntityTypeIsVisible( Cyclone::Core::Component::EntityType inType ) const				{ auto it = sFindIn( mEntityTypeVisible, inType ); return it ? it : nullptr; }

		bool *					GetEntityCategoryIsSelectable( Cyclone::Core::Component::EntityCategory inType )		{ auto it = sFindIn( mEntityCategorySelectable, inType ); return it ? it : nullptr; }
		const bool *			GetEntityCategoryIsSelectable( Cyclone::Core::Component::EntityCategory inType ) const	{ auto it = sFindIn( mEntityCategorySelectable, inType ); return it ? it : nullptr; }

		bool *					GetEntityCategoryIsVisible( Cyclone::Core::Component::EntityCategory inType )			{ auto it = sFindIn( mEntityCategoryVisible, inType ); return it ? it : nullptr; }
		const bool *			GetEntityCategoryIsVisible( Cyclone::Core::Component::EntityCategory inType ) const		{ auto it = sFindIn( mEntityCategoryVisible, inType ); return it ? it : nullptr; }

		entt::entity CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition ) const
		{
			auto type = entt::resolve( inType );
			if ( !type ) {
				assert( !"Failed to create entity: unknown type" );
				return entt::null;
			}
			
			auto func = type.func( "create_entity"_hs );
			if ( !func ) {
				assert( !"Failed to create entity: type has no create_entity function" );
				return entt::null;
			}
			
			auto result = func.invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( inPosition ) );
			if ( !result ) {
				assert( !"Failed to create entity: type has incorrect create_entity function, please report!" );
				return entt::null;
			}
			
			return result.cast<entt::entity>();
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

			T::sRegister();
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

		template<typename T>
		static const T *sFindIn( const std::vector<HashPair<T>> &inVector, auto inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( inVector.begin(), inVector.end(), hash );
			if ( it != inVector.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		template<typename T>
		static T *sFindIn( std::vector<HashPair<T>> &inVector, auto inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( inVector.begin(), inVector.end(), hash );
			if ( it != inVector.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		std::vector<HashPair<uint32_t>>		mEntityTypeColorMap;
		std::vector<HashPair<const char *>>	mEntityTypeNameMap;
		std::vector<HashPair<const char *>>	mEntityCategoryNameMap;

		std::vector<HashPair<bool>>			mEntityTypeSelectable;
		std::vector<HashPair<bool>>			mEntityTypeVisible;

		std::vector<HashPair<bool>>			mEntityCategorySelectable;
		std::vector<HashPair<bool>>			mEntityCategoryVisible;
	};
}