#pragma once

#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Math
{
	struct alignas( 32 ) Matrix44D
	{
		__m256d mR[4];

		constexpr explicit Matrix44D( std::nullptr_t ) {}
		constexpr Matrix44D( __m256d inR0, __m256d inR1, __m256d inR2, __m256d inR3 ) : mR{ inR0, inR1, inR2, inR3 } {}
		explicit Matrix44D( __m128 inR0, __m128 inR1, __m128 inR2, __m128 inR3 )
		{
			mR[0] = _mm256_cvtps_pd( inR0 );
			mR[1] = _mm256_cvtps_pd( inR1 );
			mR[2] = _mm256_cvtps_pd( inR2 );
			mR[3] = _mm256_cvtps_pd( inR3 );
		}

		static Matrix44D XM_CALLCONV sFromXMMATRIX( DirectX::FXMMATRIX inV ) { return Matrix44D( inV.r[0], inV.r[1], inV.r[2], inV.r[3] ); }
		DirectX::XMMATRIX XM_CALLCONV ToXMMATRIX() const
		{
			return DirectX::XMMATRIX(
				_mm256_cvtpd_ps( mR[0] ),
				_mm256_cvtpd_ps( mR[1] ),
				_mm256_cvtpd_ps( mR[2] ),
				_mm256_cvtpd_ps( mR[3] )
			);
		}

		Vector4D XM_CALLCONV TransformCoord3( Vector4D inV ) const
		{
			Vector4D X = _mm256_permute4x64_pd( inV, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			Vector4D Y = _mm256_permute4x64_pd( inV, _MM_SHUFFLE( 1, 1, 1, 1 ) );
			Vector4D Z = _mm256_permute4x64_pd( inV, _MM_SHUFFLE( 2, 2, 2, 2 ) );

			Vector4D R = _mm256_fmadd_pd( Z, mR[2], mR[3] );
			R = _mm256_fmadd_pd( Y, mR[1], R );
			R = _mm256_fmadd_pd( X, mR[0], R );

			Vector4D RW = _mm256_permute4x64_pd( R, _MM_SHUFFLE( 3, 3, 3, 3 ) );
			return R / RW;
		}

		Vector4D XM_CALLCONV TransformCoord3Unit( Vector4D inV ) const
		{
			Vector4D X = _mm256_permute4x64_pd( inV, _MM_SHUFFLE( 0, 0, 0, 0 ) );
			Vector4D Y = _mm256_permute4x64_pd( inV, _MM_SHUFFLE( 1, 1, 1, 1 ) );
			Vector4D Z = _mm256_permute4x64_pd( inV, _MM_SHUFFLE( 2, 2, 2, 2 ) );

			Vector4D R = _mm256_fmadd_pd( Z, mR[2], mR[3] );
			R = _mm256_fmadd_pd( Y, mR[1], R );
			R = _mm256_fmadd_pd( X, mR[0], R );

			assert( R.GetW() == 1.0 );

			return R;
		}
	};
}