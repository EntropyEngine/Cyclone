#include "pch.h"
#include "Cyclone/Core/EntityManager.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"
#include "Cyclone/Core/Component/Visible.hpp"

using Cyclone::Util::HashPair;

void Cyclone::Core::EntityManager::SetEntityTypeIsSelectable( entt::registry &inRegistry, Component::EntityType inType, bool inV )
{
	auto currentValue = mEntityTypeSelectable.Find( inType );
	if ( *currentValue == inV ) return;
	*currentValue = inV;

	BeginAction();
	mUndoStack[mUndoStackEpoch + 1].mEntityTypeSelectable = Cyclone::Util::HashPair( static_cast<entt::id_type>( inType ), inV );
	EndAction( inRegistry );
}

void Cyclone::Core::EntityManager::SetEntityTypeIsVisible( entt::registry &inRegistry, Component::EntityType inType, bool inV )
{
	auto currentValue = mEntityTypeVisible.Find( inType );
	if ( *currentValue == inV ) return;
	*currentValue = inV;

	BeginAction();
	mUndoStack[mUndoStackEpoch + 1].mEntityTypeVisible = Cyclone::Util::HashPair( static_cast<entt::id_type>( inType ), inV );
	EndAction( inRegistry );
}

void Cyclone::Core::EntityManager::SetEntityCategoryIsSelectable( entt::registry &inRegistry, Component::EntityCategory inType, bool inV )
{
	auto currentValue = mEntityCategorySelectable.Find( inType );
	if ( *currentValue == inV ) return;
	*currentValue = inV;

	BeginAction();
	mUndoStack[mUndoStackEpoch + 1].mEntityCategorySelectable = Cyclone::Util::HashPair( static_cast<entt::id_type>( inType ), inV );
	EndAction( inRegistry );
}

void Cyclone::Core::EntityManager::SetEntityCategoryIsVisible( entt::registry &inRegistry, Component::EntityCategory inType, bool inV )
{
	auto currentValue = mEntityCategoryVisible.Find( inType );
	if ( *currentValue == inV ) return;
	*currentValue = inV;

	BeginAction();
	mUndoStack[mUndoStackEpoch + 1].mEntityCategoryVisible = Cyclone::Util::HashPair( static_cast<entt::id_type>( inType ), inV );
	EndAction( inRegistry );
}

void Cyclone::Core::EntityManager::BeginAction()
{
	assert( !mUndoStackLock && "Cannot begin action while stack lock is held!" );
	mUndoStackLock = std::unique_lock( mUndoStackMutex );

	assert( mUpdatedEntities.empty() && "Updated Entities queue must be empty!" );

	if ( mUndoStackEpoch + 1 != mUndoStack.size() ) {
		mUndoStack.erase( mUndoStack.begin() + mUndoStackEpoch + 1, mUndoStack.end() );
		mUndoStack.shrink_to_fit();
	}

	mUndoStack.emplace_back();
}

void Cyclone::Core::EntityManager::EndAction( entt::registry &inRegistry )
{
	assert( mUndoStackLock && "Cannot end action with no stack lock held!" );

	for ( entt::entity entity : mUpdatedEntities ) {
		UpdateEntityInternal( entity, inRegistry );
	}
	mUpdatedEntities.clear();

	for ( entt::entity entity : mDeletedEntities ) {
		DeleteEntityInternal( entity, inRegistry );
	}
	mDeletedEntities.clear();

	ValidateSelection( inRegistry );
	UpdateVisibilityTags( inRegistry );

	mSelectionTool.mDirty = false;

	mUndoStackEpoch = static_cast<Component::EpochNumber>( mUndoStackEpoch + 1 );

	HistoryAction &currentTop = mUndoStack[mUndoStackEpoch];
	currentTop.mSelectedEntity = mSelectionTool.mSelectedEntity;
	currentTop.mSelectedEntities = mSelectionTool.mSelectedEntities;

	mUndoStackLock.unlock();
}

void Cyclone::Core::EntityManager::UndoAction( entt::registry &inRegistry )
{
	if ( mUndoStackEpoch == 0 ) return;

	assert( !mUndoStackLock && "Cannot undo action while stack lock is held!" );
	mUndoStackLock = std::unique_lock( mUndoStackMutex );

	RestoreContextStatePreUndo();

	// TODO: fix when no EpochNumber present
	const entt::registry &currentTop = mUndoStack[mUndoStackEpoch].mRegistry;
	const auto &currentTopView = currentTop.view<Component::EpochNumber>();

	for ( const entt::entity entity : currentTopView ) {
		size_t lastModifiedEpochIdx = currentTopView.get<Component::EpochNumber>( entity );

		const entt::registry &lastModifiedEpochRegistry = mUndoStack[lastModifiedEpochIdx].mRegistry;

		// TODO: fix in case of transmutation (no idea how, fuck it we ball)
		// TODO: do we get current type, or previous type?
		const auto previousType = static_cast<entt::id_type>( lastModifiedEpochRegistry.get<Component::EntityType>( entity ) );
		entt::resolve( mEntityMetaContext, previousType ).func( "restore_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( lastModifiedEpochRegistry ), entity );
		inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( lastModifiedEpochIdx ) );
	}

	for ( const entt::entity entity : currentTopView ) {
		auto func = entt::resolve( mEntityMetaContext, static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( entity ) ) ).func( "synchronise_auxiliary_components"_hs );
		if ( func ) {
			func.invoke( {}, entt::forward_as_meta( inRegistry ), entity );
		}
	}

	const auto &currentTopViewDelete = currentTop.view<Component::EntityType>( entt::exclude<Component::EpochNumber> );
	for ( const entt::entity entity : currentTopViewDelete ) {
		inRegistry.destroy( entity );
		[[maybe_unused]] entt::entity created = inRegistry.create( entity );
		assert( created == entity );
	}

	mUndoStackEpoch = static_cast<Component::EpochNumber>( mUndoStackEpoch - 1 );
	RestoreContextStatePostAction();

	ValidateSelection( inRegistry );
	UpdateVisibilityTags( inRegistry );
	
	mUndoStackLock.unlock();
}

void Cyclone::Core::EntityManager::RedoAction( entt::registry & inRegistry )
{
	if ( mUndoStackEpoch + 1 >= mUndoStack.size() ) return;

	assert( !mUndoStackLock && "Cannot redo action while stack lock is held!" );
	mUndoStackLock = std::unique_lock( mUndoStackMutex );

	size_t nextTopEpoch = mUndoStackEpoch + 1;

	const entt::registry &nextTop = mUndoStack[nextTopEpoch].mRegistry;

	// TODO: fix when no EpochNumber present
	const auto &nextTopView = nextTop.view<Component::EntityType>();

	for ( const entt::entity entity : nextTopView ) {
		// TODO: fix in case of transmutation (no idea how, fuck it we ball)
		// TODO: do we get current type, or previous type?
		const auto nextType = static_cast<entt::id_type>( nextTopView.get<Component::EntityType>( entity ) );
		entt::resolve( mEntityMetaContext, nextType ).func( "restore_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( nextTop ), entity );
		inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( nextTopEpoch ) );
	}

	for ( const entt::entity entity : nextTopView ) {
		auto func = entt::resolve( mEntityMetaContext, static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( entity ) ) ).func( "synchronise_auxiliary_components"_hs );
		if ( func ) {
			func.invoke( {}, entt::forward_as_meta( inRegistry ), entity );
		}
	}

	const auto nextTopDeletedView = nextTop.view<Component::EpochNumber>( entt::exclude<Component::EntityType> );

	for ( const entt::entity entity : nextTopDeletedView ) {
		// Ensure entity stays orphaned, not deleted
		inRegistry.destroy( entity );
		[[maybe_unused]] entt::entity created = inRegistry.create( entity );
		assert( created == entity );
	}

	mUndoStackEpoch = static_cast<Component::EpochNumber>( mUndoStackEpoch + 1 );
	RestoreContextStatePostAction();

	ValidateSelection( inRegistry );
	UpdateVisibilityTags( inRegistry );
	
	mUndoStackLock.unlock();
}

entt::entity Cyclone::Core::EntityManager::CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
{
	assert( mUndoStackLock && "Can only create entities within Begin()/End()" );

	size_t epochToUpdate = mUndoStackEpoch + 1;

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

	entt::entity entity = result.cast<entt::entity>();

	type.func( "save_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( mUndoStack[epochToUpdate].mRegistry ), entity);
	inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( epochToUpdate ) );

	auto syncFunc = entt::resolve( mEntityMetaContext, static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( entity ) ) ).func( "synchronise_auxiliary_components"_hs );
	if ( syncFunc ) {
		syncFunc.invoke( {}, entt::forward_as_meta( inRegistry ), entity );
	}

	return entity;
}

void Cyclone::Core::EntityManager::UpdateEntity( entt::entity inEntity, entt::registry &inRegistry )
{
	assert( mUndoStackLock && "Can only update entities within Begin()/End()" );
	mUpdatedEntities.insert( inEntity );
}

void Cyclone::Core::EntityManager::UpdateEntityInternal( entt::entity inEntity, entt::registry &inRegistry )
{
	size_t epochToUpdate = mUndoStackEpoch + 1;

	const auto type = static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( inEntity ) );

	entt::resolve( mEntityMetaContext, type ).func( "save_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( mUndoStack[epochToUpdate].mRegistry ), inEntity );

	// Check if the entity was created this epoch, if so, don't emplace epoch number in registry
	const auto currentEntityEpochNumber = inRegistry.get<Component::EpochNumber>( inEntity );
	if ( currentEntityEpochNumber != static_cast<Component::EpochNumber>( epochToUpdate ) ) {
		mUndoStack[epochToUpdate].mRegistry.emplace_or_replace<Component::EpochNumber>( inEntity, currentEntityEpochNumber );
		inRegistry.emplace_or_replace<Component::EpochNumber>( inEntity, static_cast<Component::EpochNumber>( epochToUpdate ) );
	}
}

void Cyclone::Core::EntityManager::DeleteEntity( entt::entity inEntity, entt::registry &inRegistry )
{
	assert( mUndoStackLock && "Can only delete entities within Begin()/End()" );

	mDeletedEntities.insert( inEntity );

	auto func = entt::resolve( mEntityMetaContext, static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( inEntity ) ) ).func( "on_delete"_hs );
	if ( func ) {
		func.invoke( {}, entt::forward_as_meta( inRegistry ), inEntity, entt::forward_as_meta( mUpdatedEntities ) );
	}
}

void Cyclone::Core::EntityManager::DeleteEntityInternal( entt::entity inEntity, entt::registry &inRegistry )
{
	size_t epochToUpdate = mUndoStackEpoch + 1;

	entt::registry &currentTop = mUndoStack[epochToUpdate].mRegistry; 

	// Create "tombston" in undo stack if non existent
	if ( !currentTop.valid( inEntity ) ) {
		[[maybe_unused]] entt::entity retEntity = currentTop.create( inEntity );
		assert( retEntity == inEntity );
	}

	// Add epoch number to tombstone
	currentTop.emplace_or_replace<Component::EpochNumber>( inEntity, inRegistry.get<Component::EpochNumber>( inEntity ) );

	// Ensure entity stays orphaned, not deleted
	inRegistry.destroy( inEntity );
	[[maybe_unused]] entt::entity created = inRegistry.create( inEntity );
	assert( created == inEntity );
}

void Cyclone::Core::EntityManager::BeginCloneAction( entt::registry &inRegistry )
{
	std::vector<entt::entity> clonedEntities;
	clonedEntities.reserve( mSelectionTool.GetSelectedEntities().size() );

	entt::entity newSelectedEntity = entt::null;

	BeginAction();

	for ( entt::entity entity : mSelectionTool.GetSelectedEntities() ) {
		entt::entity newEntity = CopyEntity( entity, inRegistry );

		if ( entity == mSelectionTool.GetSelectedEntity() ) {
			newSelectedEntity = newEntity;
		}

		clonedEntities.push_back( newEntity );
	}

	for ( entt::entity entity : clonedEntities ) {
		auto func = entt::resolve( mEntityMetaContext, static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( entity ) ) ).func( "synchronise_auxiliary_components"_hs );
		if ( func ) {
			func.invoke( {}, entt::forward_as_meta( inRegistry ), entity );
		}
	}

	mSelectionTool.ClearSelection();

	for ( entt::entity entity : clonedEntities ) {
		mSelectionTool.AddSelectedEntity( entity );
	}

	mSelectionTool.AddSelectedEntity( newSelectedEntity );

	ValidateSelection( inRegistry );
	UpdateVisibilityTags( inRegistry );
}

entt::entity Cyclone::Core::EntityManager::CopyEntity( entt::entity inEntity, entt::registry &inRegistry )
{
	assert( mUndoStackLock && "Can only clone entities within Begin()/End()" );

	size_t epochToUpdate = mUndoStackEpoch + 1;

	entt::id_type entityType = static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( inEntity ) );

	auto type = entt::resolve( mEntityMetaContext, entityType );
	if ( !type ) {
		assert( !"Failed to clone entity: unknown type" );
		return entt::null;
	}

	auto func = type.func( "clone_entity"_hs );
	if ( !func ) {
		assert( !"Failed to clone entity: type has no clone_entity function" );
		return entt::null;
	}

	entt::entity entity = inRegistry.create();

	auto result = func.invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( inEntity ), entt::forward_as_meta( entity ) );
	if ( !result ) {
		assert( !"Failed to clone entity: type has incorrect clone_entity function, please report!" );
		return entt::null;
	}

	type.func( "save_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( mUndoStack[epochToUpdate].mRegistry ), entity );
	inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( epochToUpdate ) );

	return entity;
}

void Cyclone::Core::EntityManager::RestoreContextStatePreUndo()
{
	const HistoryAction &currentTop = mUndoStack[mUndoStackEpoch];

	if ( currentTop.mEntityTypeSelectable ) *mEntityTypeSelectable.Find( currentTop.mEntityTypeSelectable.mKey ) = !currentTop.mEntityTypeSelectable.mValue;
	if ( currentTop.mEntityTypeVisible ) *mEntityTypeVisible.Find( currentTop.mEntityTypeVisible.mKey ) = !currentTop.mEntityTypeVisible.mValue;
	if ( currentTop.mEntityCategorySelectable ) *mEntityCategorySelectable.Find( currentTop.mEntityCategorySelectable.mKey ) = !currentTop.mEntityCategorySelectable.mValue;
	if ( currentTop.mEntityCategoryVisible ) *mEntityCategoryVisible.Find( currentTop.mEntityCategoryVisible.mKey ) = !currentTop.mEntityCategoryVisible.mValue;
}

void Cyclone::Core::EntityManager::RestoreContextStatePostAction()
{
	const auto &newTop = mUndoStack[mUndoStackEpoch];

	if ( newTop.mEntityTypeSelectable ) *mEntityTypeSelectable.Find( newTop.mEntityTypeSelectable.mKey ) = newTop.mEntityTypeSelectable.mValue;
	if ( newTop.mEntityTypeVisible ) *mEntityTypeVisible.Find( newTop.mEntityTypeVisible.mKey ) = newTop.mEntityTypeVisible.mValue;
	if ( newTop.mEntityCategorySelectable ) *mEntityCategorySelectable.Find( newTop.mEntityCategorySelectable.mKey ) = newTop.mEntityCategorySelectable.mValue;
	if ( newTop.mEntityCategoryVisible ) *mEntityCategoryVisible.Find( newTop.mEntityCategoryVisible.mKey ) = newTop.mEntityCategoryVisible.mValue;

	mSelectionTool.mSelectedEntity = newTop.mSelectedEntity;
	mSelectionTool.mSelectedEntities = newTop.mSelectedEntities;
}

bool Cyclone::Core::EntityManager::IsSelectionModified() const
{
	return mSelectionTool.GetSelectedEntities() != mUndoStack[mUndoStackEpoch].mSelectedEntities || mSelectionTool.GetSelectedEntity() != mUndoStack[mUndoStackEpoch].mSelectedEntity;
}

void Cyclone::Core::EntityManager::ValidateSelection( entt::registry & inRegistry )
{
	// Clear selected tags
	inRegistry.clear<entt::tag<"is_selected"_hs>>();

	// NOT A REFERENCE
	const auto previousSelection = mSelectionTool.GetSelectedEntities();

	// Ensure selection is viable
	auto view = inRegistry.view<Component::EntityType, Component::EntityCategory, Component::Visible, Component::Selectable>();
	for ( const entt::entity entity : previousSelection ) {
		if ( !view.contains( entity ) ) {
			mSelectionTool.DeselectEntity( entity );
			continue;
		}

		const auto entityCategory = view.get<Component::EntityCategory>( entity );

		if ( !GetEntityCategoryIsVisible( entityCategory ) ) {
			mSelectionTool.DeselectEntity( entity );
			continue;
		}

		if ( !GetEntityCategoryIsSelectable( entityCategory ) ) {
			mSelectionTool.DeselectEntity( entity );
			continue;
		}

		const auto entityType = view.get<Component::EntityType>( entity );

		if ( !GetEntityTypeIsVisible( entityType ) ) {
			mSelectionTool.DeselectEntity( entity );
			continue;
		}

		if ( !GetEntityTypeIsSelectable( entityType ) ) {
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

		inRegistry.emplace_or_replace<entt::tag<"is_selected"_hs>>( entity );
	}
}

void Cyclone::Core::EntityManager::UpdateVisibilityTags( entt::registry &inRegistry )
{
	inRegistry.clear<entt::tag<"is_visible"_hs>>();
	auto view = inRegistry.group<Component::EntityType, Component::EntityCategory, Component::Visible, Component::Selectable>();
	for ( const entt::entity entity : view ) {
		const auto &entityCategory = view.get<Component::EntityCategory>( entity );
		if ( !GetEntityCategoryIsVisible( entityCategory ) ) continue;

		const auto &entityType = view.get<Component::EntityType>( entity );
		if ( !GetEntityTypeIsVisible( entityType ) ) continue;

		if ( !static_cast<bool>( view.get<Component::Visible>( entity ) ) ) continue;

		inRegistry.emplace<entt::tag<"is_visible"_hs>>( entity );
	}
}
