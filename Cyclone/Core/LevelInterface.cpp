#include "pch.h"

#include "Cyclone/Core/LevelInterface.hpp"

#include "Cyclone/Core/Entity/PointDebug.hpp"
#include "Cyclone/Core/Entity/InfoDebug.hpp"

Cyclone::Core::LevelInterface::LevelInterface()
{
	mLevel = std::make_unique<Level>();
	mSelectionTool.ClearSelection();
}

void Cyclone::Core::LevelInterface::Initialize()
{
	mLevel->Initialize();
	mEntityContext.Register();

	mSelectionTool.ClearSelection();

	Entity::PointDebug().Create( GetRegistry(), { 0.0, 0.0, 0.0 } );
	Entity::PointDebug().Create( GetRegistry(), { 0.0, 0.0, 2.0 } );
	Entity::PointDebug().Create( GetRegistry(), { 0.0, 2.0, 0.0 } );
	Entity::PointDebug().Create( GetRegistry(), { 2.0, 0.0, 0.0 } );

	Entity::InfoDebug().Create( GetRegistry(), { -4.0, 0.0, 0.0 } );
	Entity::InfoDebug().Create( GetRegistry(), { -4.0, 0.0, 2.0 } );
	Entity::InfoDebug().Create( GetRegistry(), { -4.0, 2.0, 0.0 } );
	Entity::InfoDebug().Create( GetRegistry(), { -2.0, 0.0, 0.0 } );

	for ( int x = 0; x < 16; ++x ) {
		for ( int y = 0; y < 16; ++y ) {
			Entity::PointDebug().Create( GetRegistry(), { double( x * 2 + 16 ), 0.0, double( y * 2 + 16 ) } );
		}
	}

	GetRegistry().storage<Cyclone::Core::Component::Position>( "delta"_hs );
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