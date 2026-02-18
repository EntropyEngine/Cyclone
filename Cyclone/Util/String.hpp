#pragma once

#include <charconv>

namespace Cyclone::Util
{
	template<typename T>
	constexpr size_t MaxCharsForInt()
	{
		if constexpr ( std::is_enum_v<T> )
			return MaxCharsForInt<std::underlying_type_t<T>>();
		else
		{
			static_assert( std::is_integral_v<T> );
			constexpr size_t digits = std::numeric_limits<T>::digits10 + 1;
			return std::is_signed_v<T> ? digits + 1 : digits;
		}
	}

	template<size_t N, typename T>
	struct PrefixString
	{
		static_assert( std::is_integral_v<T> || std::is_enum_v<T>, "Must be integral or enum!" );
		char mBuf[N + MaxCharsForInt<T>()]{};

		PrefixString( char const ( &inPrefix )[N], T inValue )
		{
			std::memcpy( mBuf, inPrefix, N - 1 );

			if constexpr ( std::is_enum_v<T> )
				std::to_chars( mBuf + N - 1, mBuf + N + MaxCharsForInt<T>(), static_cast<std::underlying_type_t<T>>( inValue ) );
			else
				std::to_chars( mBuf + N - 1, mBuf + N + MaxCharsForInt<T>(), inValue );
		}

		operator const char *( ) const { return mBuf; }
		const char *Value() const { return mBuf + N - 1; }
	};
}