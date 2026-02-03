#pragma once

#include <float.h>
#include <limits.h>
#include <immintrin.h>

namespace Cyclone
{
	namespace Math
	{
		struct XLVector
		{
			union
			{
				__m256d mVector;
				double mScalar[4];
			};

			DirectX::XMVECTOR XM_CALLCONV ToXMVECTOR() const
			{
				return _mm256_cvtpd_ps( mVector );
			}
		};
	}
}