#pragma once

#include <charconv>

namespace Cyclone::Util
{
	template<size_t N>
	struct PrefixString
	{
		char mBuf[N + 10]{};

		PrefixString( char const ( &inPrefix )[N], entt::id_type inValue )
		{
			static_assert( sizeof( entt::id_type ) == sizeof( uint32_t ), "Using non vanilla id type!" );

			std::memcpy( mBuf, inPrefix, N - 1 );
			std::to_chars( mBuf + N - 1, mBuf + N + 10, inValue );
		}

		operator const char *( ) const { return mBuf; }
		const char *Value() const { return mBuf + N - 1; }
	};
}