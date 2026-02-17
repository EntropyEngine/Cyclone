#include "pch.h"
#include "Cyclone/Core/Entity/EntityContext.hpp"

// Cyclone Entities
#include "Cyclone/Core/Entity/PointDebug.hpp"
#include "Cyclone/Core/Entity/InfoDebug.hpp"

void Cyclone::Core::Entity::EntityContext::Register()
{
	RegisterEntityClass<PointDebug>();
	RegisterEntityClass<InfoDebug>();
	
	std::stable_sort( mEntityTypeNameMap.begin(), mEntityTypeNameMap.end() );
	std::stable_sort( mEntityCategoryNameMap.begin(), mEntityCategoryNameMap.end() );
	std::stable_sort( mEntityTypeColorMap.begin(), mEntityTypeColorMap.end() );

	// Makes all lists unique
	mEntityTypeNameMap.erase( std::unique( mEntityTypeNameMap.begin(), mEntityTypeNameMap.end() ), mEntityTypeNameMap.end() );
	mEntityCategoryNameMap.erase( std::unique( mEntityCategoryNameMap.begin(), mEntityCategoryNameMap.end() ), mEntityCategoryNameMap.end() );
	mEntityTypeColorMap.erase( std::unique( mEntityTypeColorMap.begin(), mEntityTypeColorMap.end() ), mEntityTypeColorMap.end() );

	for ( const auto &i : mEntityCategoryNameMap ) {
		mEntityCategorySelectable.emplace_back( i.mKey, true );
		mEntityCategoryVisible.emplace_back( i.mKey, true );
	}

	for ( const auto &i : mEntityTypeNameMap ) {
		mEntityTypeSelectable.emplace_back( i.mKey, true );
		mEntityTypeVisible.emplace_back( i.mKey, true );
	}
}
