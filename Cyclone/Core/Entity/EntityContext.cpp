#include "pch.h"
#include "Cyclone/Core/Entity/EntityContext.hpp"

// Cyclone Entities
#include "Cyclone/Core/Entity/PointDebug.hpp"
#include "Cyclone/Core/Entity/InfoDebug.hpp"

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
			case "player"_hs.value():	return {};
			case "point"_hs.value():	return Cyclone::Util::ColorU32( 0xDD, 0x18, 0xDD );
			case "prop"_hs.value():		return {};
			case "trigger"_hs.value():	return Cyclone::Util::ColorU32( 0xF8, 0x9A, 0x00 );
		}
	}

	return Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF );
}

template<typename T>
void Cyclone::Core::Entity::EntityContext::RegisterEntityClass()
{
	static_assert( std::is_base_of_v<Cyclone::Core::Entity::BaseEntity<T>, T> );

	mEntityTypeColorMap.emplace_back( T::kEntityType.value(), GetDebugColor<T>() );
	mEntityTypeNameMap.emplace_back( T::kEntityType.value(), T::kEntityType.data() );
	mEntityCategoryNameMap.emplace_back( T::kEntityCategory.value(), T::kEntityCategory.data() );

	T::sRegister( mEntityMetaContext );
}

void Cyclone::Core::Entity::EntityContext::Register()
{
	RegisterEntityClass<PointDebug>();
	RegisterEntityClass<InfoDebug>();
	
	// Sort lists into entity order
	std::stable_sort( mEntityTypeNameMap.begin(), mEntityTypeNameMap.end() );
	std::stable_sort( mEntityCategoryNameMap.begin(), mEntityCategoryNameMap.end() );
	std::stable_sort( mEntityTypeColorMap.begin(), mEntityTypeColorMap.end() );

	// Makes all lists unique
	mEntityTypeNameMap.erase( std::unique( mEntityTypeNameMap.begin(), mEntityTypeNameMap.end() ), mEntityTypeNameMap.end() );
	mEntityCategoryNameMap.erase( std::unique( mEntityCategoryNameMap.begin(), mEntityCategoryNameMap.end() ), mEntityCategoryNameMap.end() );
	mEntityTypeColorMap.erase( std::unique( mEntityTypeColorMap.begin(), mEntityTypeColorMap.end() ), mEntityTypeColorMap.end() );

	// Create selection/visibility map for categories
	for ( const auto &i : mEntityCategoryNameMap ) {
		mEntityCategorySelectable.emplace_back( i.mKey, true );
		mEntityCategoryVisible.emplace_back( i.mKey, true );
	}

	// Create selection/visibility map for types
	for ( const auto &i : mEntityTypeNameMap ) {
		mEntityTypeSelectable.emplace_back( i.mKey, true );
		mEntityTypeVisible.emplace_back( i.mKey, true );
	}
}

entt::entity Cyclone::Core::Entity::EntityContext::CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
{
	auto type = entt::resolve( mEntityMetaContext, inType );
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
