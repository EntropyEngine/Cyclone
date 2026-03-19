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

	mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 0.0, 0.0, 0.0 } );
	mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 0.0, 0.0, 2.0 } );
	mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 0.0, 2.0, 0.0 } );
	auto j = mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 2.0, 0.0, 0.0 } );

	mEntityManager.EndAction( GetRegistry() );

	mEntityManager.BeginAction();

	mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -4.0, 0.0, 0.0 } );
	mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -4.0, 0.0, 2.0 } );
	mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -4.0, 2.0, 0.0 } );
	auto i = mEntityManager.CreateEntity( "info_debug"_hs, GetRegistry(), { -2.0, 0.0, 0.0 } );

	mEntityManager.EndAction( GetRegistry() );

	mEntityManager.BeginAction();
	mEntityManager.DeleteEntity( i, GetRegistry() );
	mEntityManager.DeleteEntity( j, GetRegistry() );
	mEntityManager.EndAction( GetRegistry() );

	//mEntityManager.BeginAction();
	//for ( int x = 0; x < 128; ++x ) {
	//	for ( int y = 0; y < 128; ++y ) {
	//		mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { 2.0, 0.0, 2.0 } );
	//	}
	//}
	//mEntityManager.EndAction( GetRegistry() );
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
