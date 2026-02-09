#pragma once

namespace Cyclone::Util
{
	/// Converts 8 bit colour components to an ABGR packed integer
	inline constexpr uint32_t ColorU32( uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inA = 255 )
	{
		return ( static_cast<uint32_t>( inA ) << 24 ) | ( static_cast<uint32_t>( inB ) << 16 ) | ( static_cast<uint32_t>( inG ) << 8 ) | ( static_cast<uint32_t>( inR ) << 0 );
	}

	/// Converts 8 bit ABGR to XM RGBA
	inline DirectX::XMVECTOR XM_CALLCONV ColorU32ToXMVECTOR( uint32_t inARGB )
	{
		return DirectX::XMVECTORF32{
			static_cast<float>( ( inARGB & 0x000000FF ) >> 0 ) / 255.0f,
			static_cast<float>( ( inARGB & 0x0000FF00 ) >> 8 ) / 255.0f,
			static_cast<float>( ( inARGB & 0x00FF0000 ) >> 16 ) / 255.0f,
			static_cast<float>( ( inARGB & 0xFF000000 ) >> 24 ) / 255.0f
		};
	}
}