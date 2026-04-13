#include "pch.h"
#include "Cyclone/Core/EntityManager.hpp"

// Cyclone Entities
#include "Cyclone/Core/Entity/PointDebug.hpp"
#include "Cyclone/Core/Entity/InfoDebug.hpp"
#include "Cyclone/Core/Entity/PlayerSpawn.hpp"
#include "Cyclone/Core/Entity/PathDebug.hpp"

using Cyclone::Util::HashPair;

template<typename T>
constexpr uint32_t GetDebugColor()
{
	if constexpr ( requires { T::kDebugColor; } ) {
		return T::kDebugColor;
	}
	else {
		switch ( T::kEntityCategory.value() ) {
			case "ai"_hs.value():		return {};
			case "brush"_hs.value():	return {};
			case "camera"_hs.value():	return {};
			case "env"_hs.value():		return {};
			case "func"_hs.value():		return {};
			case "game"_hs.value():		return {};
			case "info"_hs.value():		return Cyclone::Util::ColorU32( 0x18, 0x18, 0xDD );
			case "item"_hs.value():		return {};
			case "light"_hs.value():	return {};
			case "logic"_hs.value():	return {};
			case "npc"_hs.value():		return {};
			case "path"_hs.value():		return Cyclone::Util::ColorU32( 0x70, 0x18, 0xFF );
			case "player"_hs.value():	return Cyclone::Util::ColorU32( 0x00, 0xFF, 0x00 );
			case "point"_hs.value():	return Cyclone::Util::ColorU32( 0xDD, 0x18, 0xDD );
			case "prop"_hs.value():		return {};
			case "trigger"_hs.value():	return Cyclone::Util::ColorU32( 0xF8, 0x9A, 0x00 );
		}
	}

	return Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF );
}

template<typename T>
void Cyclone::Core::EntityManager::RegisterEntityClass()
{
	static_assert( std::is_base_of_v<Cyclone::Core::Entity::BaseEntity<T>, T> );

	mEntityTypeColorMap.Insert( T::kEntityType.value(), GetDebugColor<T>() );
	mEntityTypeNameMap.Insert( T::kEntityType.value(), T::kEntityType.data() );
	mEntityCategoryNameMap.Insert( T::kEntityCategory.value(), T::kEntityCategory.data() );

	T::sRegister( mEntityMetaContext );
}

void Cyclone::Core::EntityManager::Register()
{
	RegisterEntityClass<Entity::PointDebug>();
	RegisterEntityClass<Entity::InfoDebug>();
	RegisterEntityClass<Entity::PlayerSpawn>();
	RegisterEntityClass<Entity::PathDebug>();

	// Sort lists into entity order
	mEntityTypeNameMap.Sort();
	mEntityCategoryNameMap.Sort();
	mEntityTypeColorMap.Sort();

	// Create selection/visibility map from sorted categories
	for ( const auto &i : mEntityCategoryNameMap ) {
		mEntityCategorySelectable.Insert( i.mKey, true );
		mEntityCategoryVisible.Insert( i.mKey, true );
	}

	// Create selection/visibility map from sorted types
	for ( const auto &i : mEntityTypeNameMap ) {
		mEntityTypeSelectable.Insert( i.mKey, true );
		mEntityTypeVisible.Insert( i.mKey, true );
	}

	// Create entity list for spawnable entities
	for ( const auto &i : mEntityTypeNameMap ) {
		if ( entt::resolve( mEntityMetaContext, i.mKey ).func( "create_entity"_hs ) ) {
			mEntitiesSpawnable.push_back( i );
		}

		if ( entt::resolve( mEntityMetaContext, i.mKey ).func( "create_brush"_hs ) ) {
			mEntitiesBrushable.push_back( i );
		}
	}

	// Sort spawnable/brushable entities by name
	std::stable_sort( mEntitiesSpawnable.begin(), mEntitiesSpawnable.end(), []( const auto &inLhs, const auto &inRhs ) { return std::strcmp( inLhs.mValue, inRhs.mValue ) < 0; } );
	std::stable_sort( mEntitiesBrushable.begin(), mEntitiesBrushable.end(), []( const auto &inLhs, const auto &inRhs ) { return std::strcmp( inLhs.mValue, inRhs.mValue ) < 0; } );
}