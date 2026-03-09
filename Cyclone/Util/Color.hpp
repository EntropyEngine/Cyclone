#pragma once

#include <immintrin.h>

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
		__m128i px = _mm_set1_epi32((int)inARGB);

		__m128i shifted = _mm_srlv_epi32(px, _mm_set_epi32(24, 16, 8, 0));

		__m128i channels = _mm_and_si128(shifted, _mm_set1_epi32(0xFF));

		__m128 f = _mm_cvtepi32_ps(channels);
		return _mm_mul_ps(f, _mm_set1_ps(1.0f / 255.0f));
	}
}