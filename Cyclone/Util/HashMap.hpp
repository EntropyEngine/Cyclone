#pragma once

namespace Cyclone::Util
{
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
	struct OptionalHashPair
	{
		entt::hashed_string::hash_type	mKey;
		bool							mHasValue = false;
		T								mValue;

		OptionalHashPair &operator =( const HashPair<T> &inPair ) { mKey = inPair.mKey; mValue = inPair.mValue; mHasValue = true; return *this; }
		operator HashPair<T>() const { return HashPair<T>( mKey, mValue ); }

		operator bool() const { return mHasValue; }
	};

	template<typename T>
	class HashMap
	{
	public:
		using PairType	= HashPair<T>;
		using KeyType	= entt::hashed_string::hash_type;
		using ValueType	= T;

		void Insert( KeyType inKey, ValueType inValue )
		{
			mStorage.emplace_back( inKey, inValue );
		}

		const T *Find( auto inType ) const
		{
			auto hash = static_cast<KeyType>( inType );
			const auto it = std::lower_bound( mStorage.begin(), mStorage.end(), hash );
			if ( it != mStorage.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		T *Find( auto inType )
		{
			auto hash = static_cast<KeyType>( inType );
			const auto it = std::lower_bound( mStorage.begin(), mStorage.end(), hash );
			if ( it != mStorage.end() && it->mKey == hash ) return &it->mValue;
			return nullptr;
		}

		const T FindOr( auto inType, T inDefault ) const
		{
			auto it = Find( inType );
			return it ? *it : inDefault;
		}

		void Sort()
		{
			std::stable_sort( mStorage.begin(), mStorage.end() );
			mStorage.erase( std::unique( mStorage.begin(), mStorage.end() ), mStorage.end() );
		}

		auto begin() { return mStorage.begin(); }
		auto end() { return mStorage.end(); }

	protected:
		std::vector<PairType> mStorage;
	};
}