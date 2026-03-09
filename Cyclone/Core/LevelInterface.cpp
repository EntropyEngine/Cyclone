#include "pch.h"

#include "Cyclone/Core/LevelInterface.hpp"

#include "Cyclone/Core/Entity/PointDebug.hpp"
#include "Cyclone/Core/Entity/InfoDebug.hpp"

Cyclone::Core::LevelInterface::LevelInterface()
{
	mLevel = std::make_unique<Level>();
	GetSelectionCtx().ClearSelection();
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

	mEntityManager.BeginAction();
	for ( int x = 0; x < 16; ++x ) {
		for ( int y = 0; y < 16; ++y ) {
			mEntityManager.CreateEntity( "point_debug"_hs, GetRegistry(), { double( x * 2 + 16 ), 0.0, double( y * 2 + 16 ) } );
		}
	}
	mEntityManager.EndAction( GetRegistry() );

	//entt::registry save;
	//Cyclone::Core::Entity::BaseEntity<Cyclone::Core::Entity::InfoDebug>::sSaveHistory( GetRegistry(), save, i );
	//
	//GetRegistry().get<Component::Position>( i ).mValue += Cyclone::Math::Vector4D( 0, 1, 0 );
	//
	//Cyclone::Core::Entity::BaseEntity<Cyclone::Core::Entity::InfoDebug>::sRestoreHistory( GetRegistry(), save, i );

	//__debugbreak();

	//mEntityManager.UndoAction( GetRegistry() );

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

void Cyclone::Core::LevelInterface::OnUpdateEnd()
{
	if ( mEntityManager.CanAquireActionLock() ) {
		/*
		// NOT A REFERENCE
		const auto previousSelection = mSelectionTool.GetSelectedEntities();

		const entt::registry &cregistry = GetRegistry();
		auto view = cregistry.view<Component::EntityType, Component::EntityCategory, Component::Visible, Component::Selectable>();
		for ( const entt::entity entity : previousSelection ) {

			if ( !view.contains( entity ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}

			const auto entityCategory = view.get<Component::EntityCategory>( entity );

			if ( !mEntityManager.GetEntityCategoryIsVisible( entityCategory ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}

			if ( !mEntityManager.GetEntityCategoryIsSelectable( entityCategory ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}

			const auto entityType = view.get<Component::EntityType>( entity );

			if ( !mEntityManager.GetEntityTypeIsVisible( entityType ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}

			if ( !mEntityManager.GetEntityTypeIsSelectable( entityType ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}

			if ( !static_cast<bool>( view.get<Component::Visible>( entity ) ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}

			if ( !static_cast<bool>( view.get<Component::Selectable>( entity ) ) ) {
				mSelectionTool.DeselectEntity( entity );
				continue;
			}
		}
		*/
	}
}
