#include "pch.h"

#include "Cyclone/Core/LevelInterface.hpp"

#include "Cyclone/Core/Entity/PointDebug.hpp"
#include "Cyclone/Core/Entity/InfoDebug.hpp"

Cyclone::Core::LevelInterface::LevelInterface()
{
	mLevel = std::make_unique<Level>();
	GetSelectionCtx().ClearSelection();

	mPrimitives = std::make_unique<Cyclone::Rendering::Primitives>();
}

void Cyclone::Core::LevelInterface::Initialize()
{
	mLevel->Initialize();
	mEntityManager.Register();

	GetSelectionCtx().ClearSelection();

	mEntityManager.BeginAction();

	mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 4.0, 0.0, 0.0 } );
	mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 4.0, 0.0, 2.0 } );
	mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 4.0, 2.0, 0.0 } );

	mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -4.0, 0.0, 0.0 } );
	mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -4.0, 0.0, 2.0 } );
	mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -4.0, 2.0, 0.0 } );

	mEntityManager.CreateEntity( "player_spawn"_hs, GetRegistry(), { 0.0, 0.0, 0.0 } );

	mEntityManager.EndAction( GetRegistry() );

	mEntityManager.BeginAction();

#ifdef _DEBUG
	int nent = 32;
#else
	int nent = 128;
#endif

	for ( int x = 0; x < nent; ++x ) {
		for ( int y = 0; y < nent; ++y ) {
			mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 2.0, 0.0, 2.0 } );
		}
	}
	mEntityManager.EndAction( GetRegistry() );
}

void Cyclone::Core::LevelInterface::SetDevice( ID3D11Device3 *inDevice )
{
	if ( mDevice ) {
		ReleaseResources();
	}

	mDevice = inDevice;

	mPrimitives->Initialize( inDevice );
}

void Cyclone::Core::LevelInterface::ReleaseResources()
{
	mPrimitives->Reset();

	// Iterate over all components which hold DX resources and release them
	// TODO

	// Release device
	mDevice.Reset();
}

void Cyclone::Core::LevelInterface::OnUpdateEnd()
{
	
}
