#pragma once

#include <float.h>
#include <limits.h>
#include <immintrin.h>

namespace Cyclone::Math
{
	struct alignas( 32 ) XLVector
	{
		__m256d mVector;

		XLVector( __m256d inVector ) : mVector( inVector ) {}
		XLVector( double inX, double inY, double inZ, double inW ) : mVector( _mm256_set_pd( inW, inZ, inY, inX ) ) {}
		XLVector( double inX, double inY, double inZ ) : XLVector( inX, inY, inZ, 0.0 ) {};

		static XLVector XM_CALLCONV sZero() { return _mm256_setzero_pd(); }

		template<size_t Axis> static XLVector XM_CALLCONV sZeroSetValueByIndex( double inV );
		template<> XLVector XM_CALLCONV sZeroSetValueByIndex<0>( double inV ) { return XLVector( inV, 0.0, 0.0, 0.0 ); }
		template<> XLVector XM_CALLCONV sZeroSetValueByIndex<1>( double inV ) { return XLVector( 0.0, inV, 0.0, 0.0 ); }
		template<> XLVector XM_CALLCONV sZeroSetValueByIndex<2>( double inV ) { return XLVector( 0.0, 0.0, inV, 0.0 ); }
		template<> XLVector XM_CALLCONV sZeroSetValueByIndex<3>( double inV ) { return XLVector( 0.0, 0.0, 0.0, inV ); }


		static XLVector XM_CALLCONV sReplicate( double inV ) { return _mm256_set1_pd( inV ); }

		static XLVector XM_CALLCONV sMin( XLVector inLhs, XLVector inRhs ) { return _mm256_min_pd( inLhs.mVector, inRhs.mVector ); }
		static XLVector XM_CALLCONV sMax( XLVector inLhs, XLVector inRhs ) { return _mm256_max_pd( inLhs.mVector, inRhs.mVector ); }
		static XLVector XM_CALLCONV sClamp( XLVector inV, XLVector inMin, XLVector inMax ) { return sMax( sMin( inV, inMax ), inMin ); }

		static XLVector XM_CALLCONV sRound( XLVector inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC ); }
		static XLVector XM_CALLCONV sFloor( XLVector inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC ); }
		static XLVector XM_CALLCONV sCeil( XLVector inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC ); }
		static XLVector XM_CALLCONV sTrunc( XLVector inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC ); }

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
		static XLVector XM_CALLCONV sFromXMVECTOR( DirectX::FXMVECTOR inV )
		{
			return _mm256_cvtps_pd( inV );
		}

		// Self conversion helper
		XM_CALLCONV operator __m256d() const
		{
			return mVector;
		}

		// FP64 addition
		XLVector XM_CALLCONV operator + ( XLVector inRhs ) const
		{
			return _mm256_add_pd( mVector, inRhs.mVector );
		}

		// FP64 inplace addition
		XLVector & XM_CALLCONV operator += ( XLVector inRhs )
		{
			mVector = _mm256_add_pd( mVector, inRhs.mVector );
			return *this;
		}

		// Unary negation
		XLVector XM_CALLCONV operator - () const
		{
			return _mm256_sub_pd( _mm256_setzero_pd(), mVector );
		}

		// FP64 subtration
		XLVector XM_CALLCONV operator - ( XLVector inRhs ) const
		{
			return _mm256_sub_pd( mVector, inRhs.mVector );
		}

		// FP64 inplace subtration
		XLVector & XM_CALLCONV operator -= ( XLVector inRhs )
		{
			mVector = _mm256_sub_pd( mVector, inRhs.mVector );
			return *this;
		}

		// FP64 multiplication
		XLVector XM_CALLCONV operator * ( XLVector inRhs ) const
		{
			return _mm256_mul_pd( mVector, inRhs.mVector );
		}

		// FP64 inplace multiplication
		XLVector & XM_CALLCONV operator *= ( XLVector inRhs )
		{
			mVector = _mm256_mul_pd( mVector, inRhs.mVector );
			return *this;
		}
	};
}