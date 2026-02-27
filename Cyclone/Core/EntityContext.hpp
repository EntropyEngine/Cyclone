#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/Color.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"
#include "Cyclone/Core/Component/EpochNumber.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

// STL Includes
#include <mutex>

namespace Cyclone::Core
{
	class EntityContext: public Cyclone::Util::NonCopyable
	{
	public:
		EntityContext() {}

		void Register();


		const char *			GetEntityTypeName( Component::EntityType inType ) const					{ auto it = sFindIn( mEntityTypeNameMap, inType ); return it ? *it : nullptr; }
		const char *			GetEntityCategoryName( Component::EntityCategory inType ) const			{ auto it = sFindIn( mEntityCategoryNameMap, inType ); return it ? *it : nullptr; }
		uint32_t				GetEntityTypeColor( Component::EntityType inType ) const				{ auto it = sFindIn( mEntityTypeColorMap, inType ); return it ? *it : Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF ); }

		bool					GetEntityTypeIsSelectable( Component::EntityType inType ) const			{ auto it = sFindIn( mEntityTypeSelectable, inType ); return it ? *it : false; }
		void					SetEntityTypeIsSelectable( Component::EntityType inType, bool inV );

		bool					GetEntityTypeIsVisible( Component::EntityType inType ) const			{ auto it = sFindIn( mEntityTypeVisible, inType ); return it ? *it : false; }
		void					SetEntityTypeIsVisible( Component::EntityType inType, bool inV );

		bool					GetEntityCategoryIsSelectable( Component::EntityCategory inType ) const	{ auto it = sFindIn( mEntityCategorySelectable, inType ); return it ? *it : false; }
		void					SetEntityCategoryIsSelectable( Component::EntityCategory inType, bool inV );

		bool					GetEntityCategoryIsVisible( Component::EntityCategory inType ) const	{ auto it = sFindIn( mEntityCategoryVisible, inType ); return it ? *it : false; }
		void					SetEntityCategoryIsVisible( Component::EntityCategory inType, bool inV );

		bool					CanAquireActionLock() const	{ return !mUndoStackLock; }
		void					BeginAction();
		void					EndAction();

		void					UndoAction( entt::registry &inRegistry );
		void					RedoAction( entt::registry &inRegistry );

		entt::entity			CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition );
		void					UpdateEntity( entt::entity inEntity, entt::registry &inRegistry );
		void					DeleteEntity( entt::entity inEntity, entt::registry &inRegistry );


		size_t					GetUndoEpoch() const { return static_cast<size_t>( mUndoStackEpoch ); }
		const auto &			GetUndoStack() const { return mUndoStack; }

	protected:
		template<typename T>
		void RegisterEntityClass();

		template<typename T>
		struct HashPair
		{
			entt::hashed_string::hash_type	mKey;
			T								mValue;
			bool operator < ( const HashPair &inRhs ) const { return mKey < inRhs.mKey; }
			bool operator ==( const HashPair &inRhs ) const { return mKey == inRhs.mKey; }
			operator entt::hashed_string::hash_type() const { return mKey; }
		};

		template<typename T>
		static const T *sFindIn( const std::vector<HashPair<T>> &inVector, auto inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( inVector.begin(), inVector.end(), hash );
			if ( it != inVector.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		template<typename T>
		static T *sFindIn( std::vector<HashPair<T>> &inVector, auto inType )
		{
			auto hash = static_cast<entt::hashed_string::hash_type>( inType );
			const auto it = std::lower_bound( inVector.begin(), inVector.end(), hash );
			if ( it != inVector.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		std::vector<HashPair<uint32_t>>		mEntityTypeColorMap;
		std::vector<HashPair<const char *>>	mEntityTypeNameMap;
		std::vector<HashPair<const char *>>	mEntityCategoryNameMap;

		std::vector<HashPair<bool>>			mEntityTypeSelectable;
		std::vector<HashPair<bool>>			mEntityTypeVisible;

		std::vector<HashPair<bool>>			mEntityCategorySelectable;
		std::vector<HashPair<bool>>			mEntityCategoryVisible;

		entt::meta_ctx						mEntityMetaContext{};

		
		std::deque<entt::registry>			mUndoStack;
		Component::EpochNumber				mUndoStackEpoch{ Component::EpochNumber::Sentinel };
		std::mutex							mUndoStackMutex;
		std::unique_lock<std::mutex>		mUndoStackLock;
	};
}