#pragma once

namespace Cyclone::Util
{
	class NonCopyable
	{
	public:
		NonCopyable() = default;
		NonCopyable( const NonCopyable & ) = delete;
		void operator = ( const NonCopyable & ) = delete;
	};
}