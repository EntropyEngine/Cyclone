#include "pch.h"

#include "Cyclone/Core/LevelInterface.hpp"

#include "Cyclone/Core/Entity/PointDebug.hpp"

Cyclone::Core::LevelInterface::LevelInterface()
{
	mLevel = std::make_unique<Level>();
	mSelectedEntity = entt::null;
}

void Cyclone::Core::LevelInterface::Initialize()
{
	mLevel->Initialize();

	mSelectedEntity = entt::null;
	mSelectedEntities.clear();

	Entity::PointDebug().Create( GetRegistry(), { 0.0, 0.0, 0.0 } );
	Entity::PointDebug().Create( GetRegistry(), { 0.0, 0.0, 2.0 } );
	Entity::PointDebug().Create( GetRegistry(), { 0.0, 2.0, 0.0 } );
	Entity::PointDebug().Create( GetRegistry(), { 2.0, 0.0, 0.0 } );
}

void Cyclone::Core::LevelInterface::SetDevice( ID3D11Device3 *inDevice )
{
	if ( mDevice ) {
		ReleaseResources();
	}

	mDevice = inDevice;
}

void Cyclone::Core::LevelInterface::ReleaseResources()
{
	// Iterate over all components which hold DX resources and release them
	// TODO

	// Release device
	mDevice.Reset();
}

void Cyclone::Core::LevelInterface::SetSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.clear();
	mSelectedEntities.insert( inEntity );
}

void Cyclone::Core::LevelInterface::AddSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.insert( inEntity );
}

void Cyclone::Core::LevelInterface::DeselectEntity( entt::entity inEntity )
{
	if ( inEntity == mSelectedEntity ) mSelectedEntity = entt::null;
	mSelectedEntities.erase( inEntity );
}
