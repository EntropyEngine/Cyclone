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

			XLVector( __m256d inVector )
			{
				mVector = inVector;
			}

			XLVector( double inX, double inY, double inZ, double inW )
			{
				mVector = _mm256_set_pd( inW, inZ, inY, inW );
			}

			XLVector( double inX, double inY, double inZ ) : XLVector( inX, inY, inZ, 0.0 ) {};

			static XLVector sZero() { return _mm256_setzero_pd(); }

			template<size_t Axis>
			static XLVector sZeroSetValueByIndex( double inV );
			template<> static XLVector sZeroSetValueByIndex<0>( double inV ) { return XLVector( inV, 0.0, 0.0, 0.0 ); }
			template<> static XLVector sZeroSetValueByIndex<1>( double inV ) { return XLVector( 0.0, inV, 0.0, 0.0 ); }
			template<> static XLVector sZeroSetValueByIndex<2>( double inV ) { return XLVector( 0.0, 0.0, inV, 0.0 ); }
			template<> static XLVector sZeroSetValueByIndex<3>( double inV ) { return XLVector( 0.0, 0.0, 0.0, inV ); }

			double GetX() const { return _mm_cvtsd_f64( _mm256_castpd256_pd128( mVector ) ); }
			double GetY() const { return mScalar[1]; }
			double GetZ() const { return mScalar[2]; }
			double GetW() const { return mScalar[3]; }

			double SetX( double inX ) { mScalar[0] = inX; }
			double SetY( double inY ) { mScalar[1] = inY; }
			double SetZ( double inZ ) { mScalar[2] = inZ; }
			double SetW( double inW ) { mScalar[3] = inW; }

			DirectX::XMVECTOR XM_CALLCONV ToXMVECTOR() const
			{
				return _mm256_cvtpd_ps( mVector );
			}

			// FP64 addition
			XLVector operator + ( XLVector inRhs ) const
			{
				return _mm256_add_pd( mVector, inRhs.mVector );
			}

			// FP64 inplace addition
			XLVector &operator += ( XLVector inRhs )
			{
				mVector = _mm256_add_pd( mVector, inRhs.mVector );
				return *this;
			}

			// Unary negation
			XLVector operator - () const
			{
				return _mm256_sub_pd( _mm256_setzero_pd(), mVector );
			}

			// FP64 subtration
			XLVector operator + ( XLVector inRhs ) const
			{
				return _mm256_sub_pd( mVector, inRhs.mVector );
			}

			// FP64 inplace subtration
			XLVector &operator += ( XLVector inRhs )
			{
				mVector = _mm256_sub_pd( mVector, inRhs.mVector );
				return *this;
			}
		};
	}
}