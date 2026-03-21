#pragma once

#include <float.h>
#include <limits.h>
#include <immintrin.h>
#include <numeric>

namespace Cyclone::Math
{
	inline __m128d XM_CALLCONV HSum4( __m256d v )
	{
		__m256d swapped = _mm256_permute2f128_pd( v, v, 0x01 );
		__m256d summed  = _mm256_add_pd( v, swapped );
		__m128d lo      = _mm256_castpd256_pd128( summed );
		__m128d swap    = _mm_shuffle_pd( lo, lo, 0b01 );
		return _mm_add_pd( lo, swap );
	}

	struct alignas( 32 ) Vector4D
	{
		using scalar_type = double;
		using vector_type = __m256d;

		__m256d mVector;

		Vector4D( __m256d inVector ) : mVector( inVector ) {}
		Vector4D( double inX, double inY, double inZ, double inW ) : mVector( _mm256_set_pd( inW, inZ, inY, inX ) ) {}
		Vector4D( double inX, double inY, double inZ ) : Vector4D( inX, inY, inZ, 0.0 ) {};

		/// @name Special Constructors
		/// @{
		static Vector4D XM_CALLCONV sZero() { return _mm256_setzero_pd(); }
		static Vector4D XM_CALLCONV sPosInf() { return _mm256_set1_pd( std::numeric_limits<double>::infinity() ); }
		static Vector4D XM_CALLCONV sNegInf() { return _mm256_set1_pd( -std::numeric_limits<double>::infinity() ); }

		/// Loads an unaligned double[4] array into the vector
		static Vector4D XM_CALLCONV sLoad( const double *inD4 ) { return _mm256_loadu_pd( inD4 ); }

		template<size_t Axis> static Vector4D XM_CALLCONV sZeroSetValueByIndex( double inV );
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<0>( double inV ) { return Vector4D( inV, 0.0, 0.0, 0.0 ); }
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<1>( double inV ) { return Vector4D( 0.0, inV, 0.0, 0.0 ); }
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<2>( double inV ) { return Vector4D( 0.0, 0.0, inV, 0.0 ); }
		template<> Vector4D XM_CALLCONV sZeroSetValueByIndex<3>( double inV ) { return Vector4D( 0.0, 0.0, 0.0, inV ); }

		/// Creates a vector with the input value broadcast to all components
		static Vector4D XM_CALLCONV sReplicate( double inV ) { return _mm256_set1_pd( inV ); }
		/// @}

		/// @name Clamping Methods
		/// @{
		static Vector4D XM_CALLCONV sMin( Vector4D inLhs, Vector4D inRhs ) { return _mm256_min_pd( inLhs.mVector, inRhs.mVector ); }
		static Vector4D XM_CALLCONV sMax( Vector4D inLhs, Vector4D inRhs ) { return _mm256_max_pd( inLhs.mVector, inRhs.mVector ); }
		static Vector4D XM_CALLCONV sClamp( Vector4D inV, Vector4D inMin, Vector4D inMax ) { return sMax( sMin( inV, inMax ), inMin ); }
		/// @}

		/// @name Rounding Methods
		/// @{
		static Vector4D XM_CALLCONV sRound( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC ); }
		static Vector4D XM_CALLCONV sFloor( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC ); }
		static Vector4D XM_CALLCONV sCeil( Vector4D inV )  { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC ); }
		static Vector4D XM_CALLCONV sTrunc( Vector4D inV ) { return _mm256_round_pd( inV.mVector, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC ); }
		/// @}

		/// @name Scalar Getters
		/// @{
		double XM_CALLCONV GetX() const { return _mm_cvtsd_f64( _mm256_castpd256_pd128( mVector ) ); }
		double XM_CALLCONV GetY() const { return _mm_cvtsd_f64( _mm_permute_pd( _mm256_castpd256_pd128( mVector ), 0x01 ) ); }
		double XM_CALLCONV GetZ() const { return _mm_cvtsd_f64( _mm256_extractf128_pd( mVector, 1 ) ); }
		double XM_CALLCONV GetW() const { return _mm_cvtsd_f64( _mm_permute_pd( _mm256_extractf128_pd( mVector, 1 ), 0x01 ) ); }

		template<size_t Axis> double XM_CALLCONV Get() const;
		template<> double XM_CALLCONV Get<0>() const { return GetX(); }
		template<> double XM_CALLCONV Get<1>() const { return GetY(); }
		template<> double XM_CALLCONV Get<2>() const { return GetZ(); }
		template<> double XM_CALLCONV Get<3>() const { return GetW(); }
		/// @}

		/// @name Scalar Setters
		/// @{
		void XM_CALLCONV SetX( double inX ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inX ), 1 ); }
		void XM_CALLCONV SetY( double inY ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inY ), 2 ); }
		void XM_CALLCONV SetZ( double inZ ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inZ ), 4 ); }
		void XM_CALLCONV SetW( double inW ) { mVector = _mm256_blend_pd( mVector, _mm256_set1_pd( inW ), 8 ); }

		template<size_t Axis> void XM_CALLCONV Set( double inV );
		template<> void XM_CALLCONV Set<0>( double inV ) { SetX( inV ); }
		template<> void XM_CALLCONV Set<1>( double inV ) { SetY( inV ); }
		template<> void XM_CALLCONV Set<2>( double inV ) { SetZ( inV ); }
		template<> void XM_CALLCONV Set<3>( double inV ) { SetW( inV ); }
		/// @}

		/// @name Comparison Functions
		/// @{
		static Vector4D XM_CALLCONV sEqual( Vector4D inLhs, Vector4D inRhs )		{ return _mm256_cmp_pd( inLhs, inRhs, _CMP_EQ_OQ ); } /// @note returns bitmask
		static Vector4D XM_CALLCONV sNotEqual( Vector4D inLhs, Vector4D inRhs )		{ return _mm256_cmp_pd( inLhs, inRhs, _CMP_NEQ_OS ); } /// @note returns bitmask
		static Vector4D XM_CALLCONV sLess( Vector4D inLhs, Vector4D inRhs )			{ return _mm256_cmp_pd( inLhs, inRhs, _CMP_LT_OQ ); } /// @note returns bitmask
		static Vector4D XM_CALLCONV sLessEqual( Vector4D inLhs, Vector4D inRhs )	{ return _mm256_cmp_pd( inLhs, inRhs, _CMP_LE_OQ ); } /// @note returns bitmask
		static Vector4D XM_CALLCONV sGreater( Vector4D inLhs, Vector4D inRhs )		{ return _mm256_cmp_pd( inLhs, inRhs, _CMP_GT_OQ ); } /// @note returns bitmask
		static Vector4D XM_CALLCONV sGreaterEqual( Vector4D inLhs, Vector4D inRhs )	{ return _mm256_cmp_pd( inLhs, inRhs, _CMP_GE_OQ ); } /// @note returns bitmask

		static Vector4D XM_CALLCONV sBitwiseOr( Vector4D inLhs, Vector4D inRhs ) { return _mm256_or_pd( inLhs, inRhs ); } /// @note returns bitmask
		static bool XM_CALLCONV sAnyTrue( Vector4D inV ) { return _mm256_movemask_pd( inV ) != 0x0; }
		/// @}

		/// @name Dot Products
		/// @{
		__m128d XM_CALLCONV Dot4V( Vector4D inV ) const
		{
			return HSum4( _mm256_mul_pd( mVector, inV ) );
		}

		__m128d XM_CALLCONV Dot3V( Vector4D inV ) const
		{
			__m256d xyz = _mm256_blend_pd( inV, _mm256_setzero_pd(), 0b1000 );
			return HSum4( _mm256_mul_pd( mVector, xyz ) );
		}

		__m128d Dot4V() const
		{
			return HSum4( _mm256_mul_pd( mVector, mVector ) );
		}

		__m128d Dot3V() const
		{
			__m256d xyz = _mm256_blend_pd( mVector, _mm256_setzero_pd(), 0b1000 );
			return HSum4( _mm256_mul_pd( xyz, xyz ) );
		}

		double XM_CALLCONV Dot4( Vector4D inV ) const { _mm_cvtsd_f64( Dot4V( inV ) ); }
		double XM_CALLCONV Dot3( Vector4D inV ) const { _mm_cvtsd_f64( Dot3V( inV ) ); }

		double             Dot4()				const { _mm_cvtsd_f64( Dot4V() ); }
		double             Dot3()				const { _mm_cvtsd_f64( Dot3V() ); }
		/// @}

		/// @name Length and Normalization
		/// @{
		Vector4D XM_CALLCONV GetLength4V() const { return _mm256_broadcastsd_pd( _mm_sqrt_pd( Dot4V() ) ); }
		Vector4D XM_CALLCONV GetLength3V() const { return _mm256_broadcastsd_pd( _mm_sqrt_pd( Dot3V() ) ); }

		double               GetLength4()  const { return _mm_cvtsd_f64( _mm_sqrt_pd( Dot4V() ) ); }
		double               GetLength3()  const { return _mm_cvtsd_f64( _mm_sqrt_pd( Dot3V() ) ); }

		Vector4D XM_CALLCONV GetNorm4()    const { return _mm256_div_pd( mVector, GetLength4V() ); }
		Vector4D XM_CALLCONV GetNorm3()    const { return _mm256_div_pd( mVector, GetLength3V() ); } // TODO: should we nuke the 4th component? 0, 1, z or unchanged?
		/// @}

		/// @name Cast Operators
		/// @{
		DirectX::XMVECTOR XM_CALLCONV ToXMVECTOR() const { return _mm256_cvtpd_ps( mVector ); }
		static Vector4D XM_CALLCONV sFromXMVECTOR( DirectX::FXMVECTOR inV ) { return _mm256_cvtps_pd( inV ); }
		XM_CALLCONV operator __m256d() const { return mVector; }
		/// @}

		/// @name Binary Elementwise Operations
		/// @{
		Vector4D XM_CALLCONV operator + ( Vector4D inRhs ) const { return _mm256_add_pd( mVector, inRhs.mVector ); }
		Vector4D XM_CALLCONV operator - ( Vector4D inRhs ) const { return _mm256_sub_pd( mVector, inRhs.mVector ); }
		Vector4D XM_CALLCONV operator * ( Vector4D inRhs ) const { return _mm256_mul_pd( mVector, inRhs.mVector ); }
		Vector4D XM_CALLCONV operator / ( Vector4D inRhs ) const { return _mm256_div_pd( mVector, inRhs.mVector ); }
		/// @}

		/// @name Inplace Elementwise Operations
		/// @{
		Vector4D & XM_CALLCONV operator += ( Vector4D inRhs ) { mVector = _mm256_add_pd( mVector, inRhs.mVector ); return *this; }
		Vector4D & XM_CALLCONV operator -= ( Vector4D inRhs ) { mVector = _mm256_sub_pd( mVector, inRhs.mVector ); return *this; }
		Vector4D & XM_CALLCONV operator *= ( Vector4D inRhs ) { mVector = _mm256_mul_pd( mVector, inRhs.mVector ); return *this; }
		Vector4D & XM_CALLCONV operator /= ( Vector4D inRhs ) { mVector = _mm256_div_pd( mVector, inRhs.mVector ); return *this; }
		/// @}

		// Unary negation
		Vector4D XM_CALLCONV operator - () const
		{
			return _mm256_sub_pd( _mm256_setzero_pd(), mVector );
		}
	};

	inline bool XM_CALLCONV operator <( Vector4D inLhs, Vector4D inRhs )
	{
		return _mm256_movemask_epi8( _mm256_castpd_si256( Vector4D::sLess( inLhs, inRhs ) ) ) == 0xFFFFFFFF;
	}

	inline bool XM_CALLCONV operator <=( Vector4D inLhs, Vector4D inRhs )
	{
		return _mm256_movemask_epi8( _mm256_castpd_si256( Vector4D::sLessEqual( inLhs, inRhs ) ) ) == 0xFFFFFFFF;
	}

	inline bool XM_CALLCONV operator >( Vector4D inLhs, Vector4D inRhs )
	{
		return _mm256_movemask_epi8( _mm256_castpd_si256( Vector4D::sGreater( inLhs, inRhs ) ) ) == 0xFFFFFFFF;
	}

	inline bool XM_CALLCONV operator >=( Vector4D inLhs, Vector4D inRhs )
	{
		return _mm256_movemask_epi8( _mm256_castpd_si256( Vector4D::sGreaterEqual( inLhs, inRhs ) ) ) == 0xFFFFFFFF;
	}

	inline bool XM_CALLCONV operator ==( Vector4D inLhs, Vector4D inRhs )
	{
		return _mm256_movemask_epi8( _mm256_castpd_si256( Vector4D::sEqual( inLhs, inRhs ) ) ) == 0xFFFFFFFF;
	}

	inline bool XM_CALLCONV operator !=( Vector4D inLhs, Vector4D inRhs )
	{
		return _mm256_movemask_epi8( _mm256_castpd_si256( Vector4D::sNotEqual( inLhs, inRhs ) ) ) == 0xFFFFFFFF;
	}
}