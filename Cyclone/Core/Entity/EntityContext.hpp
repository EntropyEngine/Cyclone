#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/Color.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

template<typename Type>
// NOLINTNEXTLINE(*-exception-escape)
struct meta_mixin: Type {
	using allocator_type = Type::allocator_type;
	using element_type = Type::element_type;

	explicit meta_mixin(const allocator_type &allocator);
};

template<typename Type>
struct entt::storage_type<Type, entt::entity> {
	using type = meta_mixin<basic_storage<Type, entt::entity>>;
};

template<typename Type>
meta_mixin<Type>::meta_mixin(const allocator_type &allocator)
	: Type{allocator} {
	using namespace entt::literals;

	entt::meta_factory<element_type>{}
	// cross registry, same type
	.template func<entt::overload<entt::storage_for_t<element_type, entt::entity> &( const entt::id_type )>( &entt::basic_registry<entt::entity>::storage<element_type> ), entt::as_ref_t>( "storage"_hs );
}

namespace Cyclone::Core::Entity
{
	class EntityContext: public Cyclone::Util::NonCopyable
	{
	public:
		EntityContext() {}

		void Register();


		const char *			GetEntityTypeName( Cyclone::Core::Component::EntityType inType ) const					{ auto it = sFindIn( mEntityTypeNameMap, inType ); return it ? *it : nullptr; }
		const char *			GetEntityCategoryName( Cyclone::Core::Component::EntityCategory inType ) const			{ auto it = sFindIn( mEntityCategoryNameMap, inType ); return it ? *it : nullptr; }
		uint32_t				GetEntityTypeColor( Cyclone::Core::Component::EntityType inType ) const					{ auto it = sFindIn( mEntityTypeColorMap, inType ); return it ? *it : Cyclone::Util::ColorU32( 0xFF, 0xFF, 0xFF ); }

		bool *					GetEntityTypeIsSelectable( Cyclone::Core::Component::EntityType inType )				{ auto it = sFindIn( mEntityTypeSelectable, inType ); return it ? it : nullptr; }
		const bool *			GetEntityTypeIsSelectable( Cyclone::Core::Component::EntityType inType ) const			{ auto it = sFindIn( mEntityTypeSelectable, inType ); return it ? it : nullptr; }

		bool *					GetEntityTypeIsVisible( Cyclone::Core::Component::EntityType inType )					{ auto it = sFindIn( mEntityTypeVisible, inType ); return it ? it : nullptr; }
		const bool *			GetEntityTypeIsVisible( Cyclone::Core::Component::EntityType inType ) const				{ auto it = sFindIn( mEntityTypeVisible, inType ); return it ? it : nullptr; }

		bool *					GetEntityCategoryIsSelectable( Cyclone::Core::Component::EntityCategory inType )		{ auto it = sFindIn( mEntityCategorySelectable, inType ); return it ? it : nullptr; }
		const bool *			GetEntityCategoryIsSelectable( Cyclone::Core::Component::EntityCategory inType ) const	{ auto it = sFindIn( mEntityCategorySelectable, inType ); return it ? it : nullptr; }

		bool *					GetEntityCategoryIsVisible( Cyclone::Core::Component::EntityCategory inType )			{ auto it = sFindIn( mEntityCategoryVisible, inType ); return it ? it : nullptr; }
		const bool *			GetEntityCategoryIsVisible( Cyclone::Core::Component::EntityCategory inType ) const		{ auto it = sFindIn( mEntityCategoryVisible, inType ); return it ? it : nullptr; }

		entt::entity CreateEntity( entt::id_type inType, entt::registry &inRegistry, const Cyclone::Math::Vector4D inPosition );

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

		entt::meta_ctx mEntityMetaContext{};
	};
}