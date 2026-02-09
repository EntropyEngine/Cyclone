#pragma once

namespace Cyclone::Util
{
	/// Converts 8 bit colour components to an ABGR packed integer
	inline constexpr uint32_t ColorU32( uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inA = 255 )
	{
		return ( static_cast<uint32_t>( inA ) << 24 ) | ( static_cast<uint32_t>( inB ) << 16 ) | ( static_cast<uint32_t>( inG ) << 8 ) | ( static_cast<uint32_t>( inR ) << 0 );
	}
}