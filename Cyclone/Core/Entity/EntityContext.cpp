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
}
