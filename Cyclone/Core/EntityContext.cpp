#include "pch.h"
#include "Cyclone/Core/EntityContext.hpp"

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
void Cyclone::Core::EntityContext::RegisterEntityClass()
{
	static_assert( std::is_base_of_v<Cyclone::Core::Entity::BaseEntity<T>, T> );

	mEntityTypeColorMap.emplace_back( T::kEntityType.value(), GetDebugColor<T>() );
	mEntityTypeNameMap.emplace_back( T::kEntityType.value(), T::kEntityType.data() );
	mEntityCategoryNameMap.emplace_back( T::kEntityCategory.value(), T::kEntityCategory.data() );

	T::sRegister( mEntityMetaContext );
}

void Cyclone::Core::EntityContext::Register()
{
	RegisterEntityClass<Entity::PointDebug>();
	RegisterEntityClass<Entity::InfoDebug>();
	
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

	// Create entity list for spawnable entities
	for ( const auto &i : mEntityTypeNameMap ) {
		if ( entt::resolve( mEntityMetaContext, i.mKey ).func( "create_entity"_hs ) ) {
			mEntitiesSpawnable.push_back( i );
		}

		if ( entt::resolve( mEntityMetaContext, i.mKey ).func( "create_brush"_hs ) ) {
			mEntitiesBrushable.push_back( i );
		}
	}

	// Sort spawnable/brushable entities by name
	std::stable_sort( mEntitiesSpawnable.begin(), mEntitiesSpawnable.end(), []( const auto &inLhs, const auto &inRhs ) { return std::strcmp( inLhs.mValue, inRhs.mValue ) < 0; } );
	std::stable_sort( mEntitiesBrushable.begin(), mEntitiesBrushable.end(), []( const auto &inLhs, const auto &inRhs ) { return std::strcmp( inLhs.mValue, inRhs.mValue ) < 0; } );
}

void Cyclone::Core::EntityContext::SetEntityTypeIsSelectable( Component::EntityType inType, bool inV )
{
	auto currentValue = sFindIn( mEntityTypeSelectable, inType );
	if ( *currentValue == inV ) return;

	BeginAction();
	entt::registry &currentTop = mUndoStack[mUndoStackEpoch + 1];

	currentTop.ctx().emplace_as<HashPair<bool>>( "entity_type_selectable"_hs, static_cast<entt::id_type>( inType ), inV );
	*currentValue = inV;

	EndAction();
}

void Cyclone::Core::EntityContext::SetEntityTypeIsVisible( Component::EntityType inType, bool inV )
{
	auto currentValue = sFindIn( mEntityTypeVisible, inType );
	if ( *currentValue == inV ) return;

	BeginAction();
	entt::registry &currentTop = mUndoStack[mUndoStackEpoch + 1];

	currentTop.ctx().emplace_as<HashPair<bool>>( "entity_type_visible"_hs, static_cast<entt::id_type>( inType ), inV );
	*currentValue = inV;

	EndAction();
}

void Cyclone::Core::EntityContext::SetEntityCategoryIsSelectable( Component::EntityCategory inType, bool inV )
{
	auto currentValue = sFindIn( mEntityCategorySelectable, inType );
	if ( *currentValue == inV ) return;

	BeginAction();
	entt::registry &currentTop = mUndoStack[mUndoStackEpoch + 1];

	currentTop.ctx().emplace_as<HashPair<bool>>( "entity_category_selectable"_hs, static_cast<entt::id_type>( inType ), inV );
	*currentValue = inV;

	EndAction();
}

void Cyclone::Core::EntityContext::SetEntityCategoryIsVisible( Component::EntityCategory inType, bool inV )
{
	auto currentValue = sFindIn( mEntityCategoryVisible, inType );
	if ( *currentValue == inV ) return;

	BeginAction();
	entt::registry &currentTop = mUndoStack[mUndoStackEpoch + 1];

	currentTop.ctx().emplace_as<HashPair<bool>>( "entity_category_visible"_hs, static_cast<entt::id_type>( inType ), inV );
	*currentValue = inV;

	EndAction();
}

void Cyclone::Core::EntityContext::BeginAction()
{
	assert( !mUndoStackLock && "Cannot begin action while stack lock is held!" );
	mUndoStackLock = std::unique_lock( mUndoStackMutex );

	if ( mUndoStackEpoch + 1 != mUndoStack.size() ) {
		mUndoStack.erase( mUndoStack.begin() + mUndoStackEpoch + 1, mUndoStack.end() );
	}

	mUndoStack.emplace_back();
}

void Cyclone::Core::EntityContext::EndAction()
{
	assert( mUndoStackLock && "Cannot end action with no stack lock held!" );
	mUndoStackLock.unlock();

	mUndoStackEpoch = static_cast<Component::EpochNumber>( mUndoStackEpoch + 1 );
}

void Cyclone::Core::EntityContext::UndoAction( entt::registry &inRegistry )
{
	if ( mUndoStackEpoch == 0 ) return;

	assert( !mUndoStackLock && "Cannot undo action while stack lock is held!" );
	mUndoStackLock = std::unique_lock( mUndoStackMutex );

	RestoreContextStatePreUndo();

	// TODO: fix when no EpochNumber present
	const entt::registry &currentTop = mUndoStack[mUndoStackEpoch];
	const auto &currentTopView = currentTop.view<Component::EpochNumber>();

	for ( const entt::entity entity : currentTopView ) {
		size_t lastModifiedEpochIdx = currentTopView.get<Component::EpochNumber>( entity );

		const entt::registry &lastModifiedEpochRegistry = mUndoStack[lastModifiedEpochIdx];

		// TODO: fix in case of transmutation (no idea how, fuck it we ball)
		// TODO: do we get current type, or previous type?
		const auto previousType = static_cast<entt::id_type>( lastModifiedEpochRegistry.get<Component::EntityType>( entity ) );
		entt::resolve( mEntityMetaContext, previousType ).func( "restore_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( lastModifiedEpochRegistry ), entity );
		inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( lastModifiedEpochIdx ) );
	}

	const auto &currentTopViewDelete = currentTop.view<Component::EntityType>( entt::exclude<Component::EpochNumber> );
	for ( const entt::entity entity : currentTopViewDelete ) {
		inRegistry.destroy( entity );

		entt::entity created = inRegistry.create( entity );
		assert( created == entity );
	}

	mUndoStackLock.unlock();

	mUndoStackEpoch = static_cast<Component::EpochNumber>( mUndoStackEpoch - 1 );

	RestoreContextStatePostAction();
}

void Cyclone::Core::EntityContext::RedoAction( entt::registry & inRegistry )
{
	if ( mUndoStackEpoch + 1 >= mUndoStack.size() ) return;

	assert( !mUndoStackLock && "Cannot redo action while stack lock is held!" );
	mUndoStackLock = std::unique_lock( mUndoStackMutex );

	size_t nextTopEpoch = mUndoStackEpoch + 1;

	const entt::registry &nextTop = mUndoStack[nextTopEpoch];

	// TODO: fix when no EpochNumber present
	const auto &nextTopView = nextTop.view<Component::EntityType>();

	for ( const entt::entity entity : nextTopView ) {
		// TODO: fix in case of transmutation (no idea how, fuck it we ball)
		// TODO: do we get current type, or previous type?
		const auto nextType = static_cast<entt::id_type>( nextTopView.get<Component::EntityType>( entity ) );
		entt::resolve( mEntityMetaContext, nextType ).func( "restore_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( nextTop ), entity );
		inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( nextTopEpoch ) );
	}

	const auto nextTopDeletedView = nextTop.view<Component::EpochNumber>( entt::exclude<Component::EntityType> );

	for ( const entt::entity entity : nextTopDeletedView ) {
		// Ensure entity stays orphaned, not deleted
		inRegistry.destroy( entity );
		entt::entity created = inRegistry.create( entity );
		assert( created == entity );
	}

	mUndoStackLock.unlock();

	mUndoStackEpoch = static_cast<Component::EpochNumber>( mUndoStackEpoch + 1 );

	RestoreContextStatePostAction();
}

entt::entity Cyclone::Core::EntityContext::CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition )
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

	type.func( "save_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( mUndoStack[epochToUpdate] ), entity);
	inRegistry.emplace_or_replace<Component::EpochNumber>( entity, static_cast<Component::EpochNumber>( epochToUpdate ) );

	return entity;
}

void Cyclone::Core::EntityContext::UpdateEntity( entt::entity inEntity, entt::registry &inRegistry )
{
	assert( mUndoStackLock && "Can only update entities within Begin()/End()" );

	size_t epochToUpdate = mUndoStackEpoch + 1;

	const auto type = static_cast<entt::id_type>( inRegistry.get<Component::EntityType>( inEntity ) );

	entt::resolve( mEntityMetaContext, type ).func( "save_history"_hs ).invoke( {}, entt::forward_as_meta( inRegistry ), entt::forward_as_meta( mUndoStack[epochToUpdate] ), inEntity );
	mUndoStack[epochToUpdate].emplace_or_replace<Component::EpochNumber>( inEntity, inRegistry.get<Component::EpochNumber>( inEntity ) );
	inRegistry.emplace_or_replace<Component::EpochNumber>( inEntity, static_cast<Component::EpochNumber>( epochToUpdate ) );
}

void Cyclone::Core::EntityContext::DeleteEntity( entt::entity inEntity, entt::registry & inRegistry )
{
	assert( mUndoStackLock && "Can only delete entities within Begin()/End()" );

	size_t epochToUpdate = mUndoStackEpoch + 1;

	entt::registry &currentTop = mUndoStack[epochToUpdate]; 

	// Create in undo stack if non existent
	if ( !currentTop.valid( inEntity ) ) {
		auto retEntity = currentTop.create( inEntity );
		assert( retEntity == inEntity );
	}

	// Only provide epoch number
	mUndoStack[epochToUpdate].emplace_or_replace<Component::EpochNumber>( inEntity, inRegistry.get<Component::EpochNumber>( inEntity ) );

	// Ensure entity stays orphaned, not deleted
	inRegistry.destroy( inEntity );
	entt::entity created = inRegistry.create( inEntity );
	assert( created == inEntity );
}

void Cyclone::Core::EntityContext::RestoreContextStatePreUndo()
{
	const entt::registry &currentTop = mUndoStack[mUndoStackEpoch];

	const auto entityTypeSelectableCtx = currentTop.ctx().find<HashPair<bool>>( "entity_type_selectable"_hs );
	if ( entityTypeSelectableCtx ) *sFindIn( mEntityTypeSelectable, entityTypeSelectableCtx->mKey ) = !entityTypeSelectableCtx->mValue;

	const auto entityTypeVisibleCtx = currentTop.ctx().find<HashPair<bool>>( "entity_type_visible"_hs );
	if ( entityTypeVisibleCtx ) *sFindIn( mEntityTypeVisible, entityTypeVisibleCtx->mKey ) = !entityTypeVisibleCtx->mValue;

	const auto entityCategorySelectableCtx = currentTop.ctx().find<HashPair<bool>>( "entity_category_selectable"_hs );
	if ( entityCategorySelectableCtx ) *sFindIn( mEntityCategorySelectable, entityCategorySelectableCtx->mKey ) = !entityCategorySelectableCtx->mValue;

	const auto entityCategoryVisibleCtx = currentTop.ctx().find<HashPair<bool>>( "entity_category_visible"_hs );
	if ( entityCategoryVisibleCtx ) *sFindIn( mEntityCategoryVisible, entityCategoryVisibleCtx->mKey ) = !entityCategoryVisibleCtx->mValue;
}

void Cyclone::Core::EntityContext::RestoreContextStatePostAction()
{
	const auto &newTop = mUndoStack[mUndoStackEpoch];

	const auto entityTypeSelectableCtx = newTop.ctx().find<HashPair<bool>>( "entity_type_selectable"_hs );
	if ( entityTypeSelectableCtx ) *sFindIn( mEntityTypeSelectable, entityTypeSelectableCtx->mKey ) = entityTypeSelectableCtx->mValue;

	const auto entityTypeVisibleCtx = newTop.ctx().find<HashPair<bool>>( "entity_type_visible"_hs );
	if ( entityTypeVisibleCtx ) *sFindIn( mEntityTypeVisible, entityTypeVisibleCtx->mKey ) = entityTypeVisibleCtx->mValue;

	const auto entityCategorySelectableCtx = newTop.ctx().find<HashPair<bool>>( "entity_category_selectable"_hs );
	if ( entityCategorySelectableCtx ) *sFindIn( mEntityCategorySelectable, entityCategorySelectableCtx->mKey ) = entityCategorySelectableCtx->mValue;

	const auto entityCategoryVisibleCtx = newTop.ctx().find<HashPair<bool>>( "entity_category_visible"_hs );
	if ( entityCategoryVisibleCtx ) *sFindIn( mEntityCategoryVisible, entityCategoryVisibleCtx->mKey ) = entityCategoryVisibleCtx->mValue;
}
