#include "pch.h"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

#include "Cyclone/Core/Entity/PointDebug.hpp"

Cyclone::Core::Entity::EntityTypeRegistry Cyclone::Core::Entity::EntityTypeRegistry::sInstance = Cyclone::Core::Entity::EntityTypeRegistry();

Cyclone::Core::Entity::EntityTypeRegistry::EntityTypeRegistry()
{
	OutputDebugStringA( "Registering entities!\n" );
	RegisterEntityClass<PointDebug>();
}
