#pragma once

#include <float.h>
#include <limits.h>
#include <immintrin.h>

namespace Cyclone::Math
{
	struct alignas( 32 ) Vector4D
	{
		__m256d mVector;

		Vector4D( __m256d inVector ) : mVector( inVector ) {}
		Vector4D( double inX, double inY, double inZ, double inW ) : mVector( _mm256_set_pd( inW, inZ, inY, inX ) ) {}
		Vector4D( double inX, double inY, double inZ ) : Vector4D( inX, inY, inZ, 0.0 ) {};

		static Vector4D XM_CALLCONV sZero() { return _mm256_setzero_pd(); }

		template<size_t Axis> static Vector4D XM_CALLCONV sZeroSetValueByIndex( double inV );
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<0>( double inV ) { return Vector4D( inV, 0.0, 0.0, 0.0 ); }
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<1>( double inV ) { return Vector4D( 0.0, inV, 0.0, 0.0 ); }
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<2>( double inV ) { return Vector4D( 0.0, 0.0, inV, 0.0 ); }
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<3>( double inV ) { return Vector4D( 0.0, 0.0, 0.0, inV ); }


		static Vector4D XM_CALLCONV sReplicate( double inV ) { return _mm256_set1_pd( inV ); }

		static Vector4D XM_CALLCONV sMin( Vector4D inLhs, Vector4D inRhs ) { return _mm256_min_pd( inLhs.mVector, inRhs.mVector ); }
		static Vector4D XM_CALLCONV sMax( Vector4D inLhs, Vector4D inRhs ) { return _mm256_max_pd( inLhs.mVector, inRhs.mVector ); }
		static Vector4D XM_CALLCONV sClamp( Vector4D inV, Vector4D inMin, Vector4D inMax ) { return sMax( sMin( inV, inMax ), inMin ); }

		static Vector4D XM_CALLCONV sRound( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC ); }
		static Vector4D XM_CALLCONV sFloor( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC ); }
		static Vector4D XM_CALLCONV sCeil( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC ); }
		static Vector4D XM_CALLCONV sTrunc( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC ); }

		double XM_CALLCONV GetX() const { return _mm_cvtsd_f64( _mm256_castpd256_pd128( mVector ) ); }
		double XM_CALLCONV GetY() const { return _mm_cvtsd_f64( _mm_permute_pd( _mm256_castpd256_pd128( mVector ), 0x01 ) ); }
		double XM_CALLCONV GetZ() const { return _mm_cvtsd_f64( _mm256_extractf128_pd( mVector, 1 ) ); }
		double XM_CALLCONV GetW() const { return _mm_cvtsd_f64( _mm_permute_pd( _mm256_extractf128_pd( mVector, 1 ), 0x01 ) ); }

		template<size_t Axis> double XM_CALLCONV Get() const;
		template<> double XM_CALLCONV Get<0>() const { return GetX(); }
		template<> double XM_CALLCONV Get<1>() const { return GetY(); }
		template<> double XM_CALLCONV Get<2>() const { return GetZ(); }
		template<> double XM_CALLCONV Get<3>() const { return GetW(); }


		void XM_CALLCONV SetX( double inX ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inX ), 1 ); }
		void XM_CALLCONV SetY( double inY ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inY ), 2 ); }
		void XM_CALLCONV SetZ( double inZ ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inZ ), 4 ); }
		void XM_CALLCONV SetW( double inW ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inW ), 8 ); }

		template<size_t Axis> void XM_CALLCONV Set( double inV );
		template<> void XM_CALLCONV Set<0>( double inV ) { SetX( inV ); }
		template<> void XM_CALLCONV Set<1>( double inV ) { SetY( inV ); }
		template<> void XM_CALLCONV Set<2>( double inV ) { SetZ( inV ); }
		template<> void XM_CALLCONV Set<3>( double inV ) { SetW( inV ); }


		// Cast to 32 bit
		DirectX::XMVECTOR XM_CALLCONV ToXMVECTOR() const
		{
			return _mm256_cvtpd_ps( mVector );
		}

		// Cast from 32 bit
		static Vector4D XM_CALLCONV sFromXMVECTOR( DirectX::FXMVECTOR inV )
		{
			return _mm256_cvtps_pd( inV );
		}

		// Self conversion helper
		XM_CALLCONV operator __m256d() const
		{
			return mVector;
		}

		// FP64 addition
		Vector4D XM_CALLCONV operator + ( Vector4D inRhs ) const
		{
			return _mm256_add_pd( mVector, inRhs.mVector );
		}

		// FP64 inplace addition
		Vector4D & XM_CALLCONV operator += ( Vector4D inRhs )
		{
			mVector = _mm256_add_pd( mVector, inRhs.mVector );
			return *this;
		}

		// Unary negation
		Vector4D XM_CALLCONV operator - () const
		{
			return _mm256_sub_pd( _mm256_setzero_pd(), mVector );
		}

		// FP64 subtration
		Vector4D XM_CALLCONV operator - ( Vector4D inRhs ) const
		{
			return _mm256_sub_pd( mVector, inRhs.mVector );
		}

		// FP64 inplace subtration
		Vector4D & XM_CALLCONV operator -= ( Vector4D inRhs )
		{
			mVector = _mm256_sub_pd( mVector, inRhs.mVector );
			return *this;
		}

		// FP64 multiplication
		Vector4D XM_CALLCONV operator * ( Vector4D inRhs ) const
		{
			return _mm256_mul_pd( mVector, inRhs.mVector );
		}

		// FP64 inplace multiplication
		Vector4D & XM_CALLCONV operator *= ( Vector4D inRhs )
		{
			mVector = _mm256_mul_pd( mVector, inRhs.mVector );
			return *this;
		}
	};
}