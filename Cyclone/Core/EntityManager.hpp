#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/Color.hpp"
#include "Cyclone/Util/HashMap.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"
#include "Cyclone/Core/Component/EpochNumber.hpp"

// Cyclone core
#include "Cyclone/Core/HistoryAction.hpp"

// Cyclone tools
#include "Cyclone/Core/Tool/SelectionToolContext.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

// STL Includes
#include <mutex>

namespace Cyclone::Core
{
	class EntityManager: public Cyclone::Util::NonCopyable
	{
	public:
		EntityManager() {}

		void Register();


		const char *			GetEntityTypeName( Component::EntityType inType ) const					{ return mEntityTypeNameMap.FindOr( inType, nullptr ); }
		const char *			GetEntityCategoryName( Component::EntityCategory inType ) const			{ return mEntityCategoryNameMap.FindOr( inType, nullptr ); }
		uint32_t				GetEntityTypeColor( Component::EntityType inType ) const				{ return mEntityTypeColorMap.FindOr( inType, Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF ) ); }

		bool					GetEntityTypeIsSelectable( Component::EntityType inType ) const			{ return mEntityTypeSelectable.FindOr( inType, false ); }
		bool					GetEntityTypeIsVisible( Component::EntityType inType ) const			{ return mEntityTypeVisible.FindOr( inType, false ); }
		bool					GetEntityCategoryIsSelectable( Component::EntityCategory inType ) const	{ return mEntityCategorySelectable.FindOr( inType, false ); }
		bool					GetEntityCategoryIsVisible( Component::EntityCategory inType ) const	{ return mEntityCategoryVisible.FindOr( inType, false ); }

		void					SetEntityTypeIsSelectable( entt::registry &inRegistry, Component::EntityType inType, bool inV );
		void					SetEntityTypeIsVisible( entt::registry &inRegistry, Component::EntityType inType, bool inV );
		void					SetEntityCategoryIsSelectable( entt::registry &inRegistry, Component::EntityCategory inType, bool inV );
		void					SetEntityCategoryIsVisible( entt::registry &inRegistry, Component::EntityCategory inType, bool inV );

		bool					CanAquireActionLock() const	{ return !mUndoStackLock; }
		void					BeginAction();
		void					EndAction( entt::registry &inRegistry );

		void					UndoAction( entt::registry &inRegistry );
		void					RedoAction( entt::registry &inRegistry );

		entt::entity			CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition );
		void					UpdateEntity( entt::entity inEntity, entt::registry &inRegistry );
		void					DeleteEntity( entt::entity inEntity, entt::registry &inRegistry );
		void					BeginCloneAction( entt::registry &inRegistry );

		size_t					GetUndoEpoch() const { return static_cast<size_t>( mUndoStackEpoch ); }
		const auto &			GetUndoStack() const { return mUndoStack; }

		Tool::SelectionToolContext & GetSelectionCtx()					{ return mSelectionTool; }
		const Tool::SelectionToolContext & GetSelectionCtx() const		{ return mSelectionTool; }

		const std::set<entt::entity> & GetOpenedProperties() const		{ return mOpenedProperties; }
		void					OpenEntityProperties( entt::entity inEntity ) { mOpenedProperties.emplace( inEntity ); }
		void					CloseEntityProperties( entt::entity inEntity ) { mOpenedProperties.erase( inEntity ); }

		bool					IsSelectionModified() const;

		template<typename T>
		const T &				GetCanonicalComponent( entt::entity inEntity, const entt::registry &inRegistry ) const
		{
			size_t lastModifiedEpochIdx = inRegistry.get<Component::EpochNumber>( inEntity );
			const entt::registry &lastModifiedEpochRegistry = mUndoStack[lastModifiedEpochIdx].mRegistry;
			return lastModifiedEpochRegistry.get<T>( inEntity );
		}

	protected:
		template<typename T>
		void RegisterEntityClass();

		entt::entity			CopyEntity( entt::entity inEntity, entt::registry &inRegistry );
		void					UpdateEntityInternal( entt::entity inEntity, entt::registry &inRegistry );

		void RestoreContextStatePreUndo(); ///< We need to do an extra step for undo actions which flips the context state
		void RestoreContextStatePostAction();

		void ValidateSelection( entt::registry &inRegistry );
		void UpdateVisibilityTags( entt::registry &inRegistry );

		Cyclone::Util::HashMap<uint32_t>	mEntityTypeColorMap;
		Cyclone::Util::HashMap<const char *>mEntityTypeNameMap;
		Cyclone::Util::HashMap<const char *>mEntityCategoryNameMap;

		Cyclone::Util::HashMap<bool>		mEntityTypeSelectable;
		Cyclone::Util::HashMap<bool>		mEntityTypeVisible;

		Cyclone::Util::HashMap<bool>		mEntityCategorySelectable;
		Cyclone::Util::HashMap<bool>		mEntityCategoryVisible;

		std::vector<Cyclone::Util::HashPair<const char *>> mEntitiesSpawnable;
		std::vector<Cyclone::Util::HashPair<const char *>> mEntitiesBrushable;

		entt::meta_ctx						mEntityMetaContext{};

		
		std::deque<HistoryAction>			mUndoStack;
		Component::EpochNumber				mUndoStackEpoch{ Component::EpochNumber::Sentinel };
		std::mutex							mUndoStackMutex;
		std::unique_lock<std::mutex>		mUndoStackLock;

		Tool::SelectionToolContext			mSelectionTool;

		std::set<entt::entity>				mOpenedProperties;

		std::set<entt::entity>				mUpdatedEntities;
	};
}