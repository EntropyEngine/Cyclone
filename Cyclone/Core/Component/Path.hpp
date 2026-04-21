#pragma once

#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/Math/Matrix.hpp"

#include <format>

namespace Cyclone::Core::Component
{
	using PathTag = entt::tag<"path_tag"_hs>;

	struct PathData
	{
		PathData() = default;

		struct Knot
		{
			Cyclone::Math::Vector4D mPoint;
			DirectX::XMVECTOR		mInVec;
			DirectX::XMVECTOR		mOutVec;
		};

		struct Extrusion
		{
			DirectX::XMVECTOR		mNormal;
			DirectX::XMVECTOR		mBitangent;
		};

		static constexpr const char * kExtrusionTypes[] = { "Explicit", "Twist", "Tilt" };
		enum EExtrusionType : uint8_t
		{
			CurveNormal		= ( 1 << 0 ),
			CurveBitangent	= ( 1 << 1 ),
			CurveBoth		= CurveNormal | CurveBitangent, 

			EaseIn			= ( 1 << 2 ),
			EaseOut			= ( 1 << 3 ),
			EaseInOut		= EaseIn | EaseOut,

			CustomNormal	= ( 1 << 4 ),
			CustomBitangent	= ( 1 << 5 )
		};

		static constexpr const char * kTangentTypes[] = { "Split", "Aligned", "Mirrored" };
		enum class ETangentType : uint8_t
		{
			Split,
			Aligned,
			Mirrored
		};

		static constexpr const char * kSegmentTypes[] = { "Custom", "Straight", "Curve", "HalfLoop", "FullLoop", "Child" };
		enum class ESegmentType : uint8_t
		{
			Custom,
			Straight,
			Curve,
			HalfLoop,
			FullLoop,
			Child
		};

		std::vector<Knot>			mKnots;
		std::vector<Extrusion>		mExtrusions;
		std::vector<uint8_t>		mExtrusionTypes;
		std::vector<ETangentType>	mTangentType;
		std::vector<ESegmentType>	mSegmentType;
		std::vector<float>			mPathWidths;

		void AddKnot()
		{
			if ( mKnots.size() == 0 ) {
				mKnots.emplace_back( Cyclone::Math::Vector4D::sZero(), DirectX::XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f ), DirectX::XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f ) );
				mExtrusions.emplace_back( DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ), DirectX::XMVectorSet( 1.0f, 0.0f, 0.0f, 0.0f ) );
				mExtrusionTypes.push_back( EExtrusionType::CurveBoth | EExtrusionType::EaseInOut );
				mTangentType.push_back( ETangentType::Aligned );
				mPathWidths.push_back( 2.0f );
			}
			else {
				size_t i = mKnots.size() - 1;
				mKnots.emplace_back( mKnots[i].mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( mKnots[i].mOutVec ) * Cyclone::Math::Vector4D::sReplicate( 3.0f ), DirectX::XMVectorNegate( mKnots[i].mOutVec ), mKnots[i].mOutVec);
				mExtrusions.emplace_back( mExtrusions[i].mNormal, mExtrusions[i].mBitangent );
				mExtrusionTypes.push_back( EExtrusionType::CurveBoth | EExtrusionType::EaseInOut );
				mTangentType.push_back( ETangentType::Aligned );
				mPathWidths.push_back( mPathWidths.back() );

				mSegmentType.push_back( ESegmentType::Custom );
			}
		}

		void AddHalfLoop( double inT = DirectX::XM_PI, double inStride = 1.0f, double inRadius = 2.0f )
		{
			using Cyclone::Math::Vector4D;
			using Cyclone::Math::Matrix44D;

			size_t knot0 = mKnots.size() - 1;

			inStride /= inRadius;

			mExtrusionTypes[knot0] &= ~( CustomNormal | CustomBitangent );
			mTangentType[knot0] = ETangentType::Aligned;

			UpdateTangentValue( knot0, false );
			ComputeAutoExtrusions( knot0 );

			const double A = -DirectX::XM_PI / 2;
			const double D = inT -DirectX::XM_PI / 2;
			const double B = ( A + D ) / 2;

			Vector4D LP = Vector4D::sReplicate( ( 4.0 / 3.0 ) * std::tan( ( B - A ) / 4.0 ) );
			Vector4D LJ = Vector4D::sReplicate( ( 4.0 / 3.0 ) * std::tan( ( B - D ) / 4.0 ) );

			const double sinA = std::sin( A );
			const double cosA = std::cos( A );

			const double sinB = std::sin( B );
			const double cosB = std::cos( B );

			const double sinD = std::sin( D );
			const double cosD = std::cos( D );
			
			Vector4D P0( cosA, 0, sinA );
			Vector4D J0( cosD, inStride, sinD );

			Vector4D P1 = P0 + LP * Vector4D( -sinA, 0, cosA );
			Vector4D J1 = J0 + LJ * Vector4D( -sinD, 0, cosD );

			Vector4D C3( cosB, inStride / 2, sinB );

			Vector4D P2 = C3 - LP * Vector4D( -sinB, 0.0, cosB );
			P2 -= Vector4D( 0, inStride / 4, 0 ) / P2.GetLength3V();

			Vector4D J2 = C3 - LJ * Vector4D( -sinB, 0.0, cosB );
			J2 -= Vector4D( 0, -inStride / 4, 0 ) / J2.GetLength3V();

			Vector4D scale = Vector4D::sReplicate( inRadius );
			Matrix44D align(
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mKnots[knot0].mOutVec, 0 ) ).GetNorm3() * scale,
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mExtrusions[knot0].mBitangent, 0 ) ) * scale,
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mExtrusions[knot0].mNormal, 0 ) ) * scale,
				Vector4D( 0, 0, 0, 1 )
			);

			Vector4D offset( 0, 0, 1 );

			P0 = align.TransformCoord3Unit( P0 + offset ) + mKnots[knot0].mPoint;
			J0 = align.TransformCoord3Unit( J0 + offset ) + mKnots[knot0].mPoint;

			P1 = align.TransformCoord3Unit( P1 + offset ) + mKnots[knot0].mPoint;
			J1 = align.TransformCoord3Unit( J1 + offset ) + mKnots[knot0].mPoint;

			P2 = align.TransformCoord3Unit( P2 + offset ) + mKnots[knot0].mPoint;
			J2 = align.TransformCoord3Unit( J2 + offset ) + mKnots[knot0].mPoint;

			C3 = align.TransformCoord3Unit( C3 + offset ) + mKnots[knot0].mPoint;

			AddKnot();
			AddKnot();

			assert( ( mKnots[knot0].mPoint - P0 ).GetLength3() < 1e-7 );

			mKnots[knot0].mOutVec = ( P1 - P0 ).ToXMVECTOR();

			mKnots[knot0 + 1].mPoint = C3;
			mKnots[knot0 + 1].mInVec = ( P2 - C3 ).ToXMVECTOR();
			mKnots[knot0 + 1].mOutVec = ( J2 - C3 ).ToXMVECTOR();

			mExtrusions[knot0 + 1].mNormal = DirectX::XMVector3Rotate( mExtrusions[knot0].mNormal, DirectX::XMQuaternionRotationNormal( mExtrusions[knot0].mBitangent, - inT / 2 ) );

			mKnots[knot0 + 2].mPoint = J0;
			mKnots[knot0 + 2].mInVec = ( J1 - J0 ).ToXMVECTOR();
			mKnots[knot0 + 2].mOutVec = ( -J1 + J0 ).ToXMVECTOR();

			mExtrusions[knot0 + 2].mNormal = DirectX::XMVector3Rotate( mExtrusions[knot0 + 1].mNormal, DirectX::XMQuaternionRotationNormal( mExtrusions[knot0].mBitangent, - inT / 2 ) );

			mExtrusionTypes[knot0 + 1] &= ~( CustomNormal | CustomBitangent );
			mExtrusionTypes[knot0 + 2] &= ~( CustomNormal | CustomBitangent );
			mTangentType[knot0 + 1] = ETangentType::Aligned;
			mTangentType[knot0 + 2] = ETangentType::Aligned;
			ComputeAutoExtrusions( knot0 + 1, false );
			ComputeAutoExtrusions( knot0 + 2, false );

		}

		void AddFullLoop( double inT = DirectX::XM_PI, double stride = 1.0f, double radius = 2.0f )
		{
			using Cyclone::Math::Vector4D;
			using Cyclone::Math::Matrix44D;

			size_t knot0 = mKnots.size() - 1;

			mExtrusionTypes[knot0] &= ~( CustomNormal | CustomBitangent );
			mTangentType[knot0] = ETangentType::Aligned;

			UpdateTangentValue( knot0, false );
			ComputeAutoExtrusions( knot0 );

			auto c_bezier = [inT]( double u0, double u1, Vector4D *C ) {
				auto c = [inT]( double u ) {
					return Vector4D( std::sin( inT * u ), 0.5 - 0.5 * std::cos( DirectX::XM_PI * u ), -std::cos( inT * u ) );
				};

				auto c_prime = [inT]( double u ) {
					return Vector4D( inT * std::cos( inT * u ), DirectX::XM_PI * 0.5 * std::sin( DirectX::XM_PI * u ), inT * std::sin( inT * u ) );
				};

				C[0] = c( u0 );
				C[3] = c( u1 );

				Vector4D T0 = c_prime( u0 );
				Vector4D T1 = c_prime( u1 );

				Vector4D delta = Vector4D::sReplicate( ( u1 - u0 ) / 3.0 );

				C[1] = C[0] + delta * T0;
				C[2] = C[3] - delta * T1;
			};

			Vector4D C0[4] = { Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr } };
			Vector4D C1[4] = { Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr } };
			Vector4D C2[4] = { Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr } };
			Vector4D C3[4] = { Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr } };

			c_bezier( 0.0 / 4, 1.0 / 4, C0 );
			c_bezier( 1.0 / 4, 2.0 / 4, C1 );
			c_bezier( 2.0 / 4, 3.0 / 4, C2 );
			c_bezier( 3.0 / 4, 4.0 / 4, C3 );

			Vector4D *curves[4] = { C0, C1, C2, C3 };

			Vector4D fScale = Vector4D::sReplicate( std::abs( radius ) );
			Vector4D rScale = Vector4D::sReplicate( radius );
			Vector4D sScale = Vector4D::sReplicate( stride );
			Matrix44D align(
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mKnots[knot0].mOutVec, 0 ) ).GetNorm3() * fScale,
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mExtrusions[knot0].mBitangent, 0 ) ) * sScale,
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mExtrusions[knot0].mNormal, 0 ) ) * rScale,
				Vector4D( 0, 0, 0, 1 )
			);

			Vector4D offset( 0, 0, 1 );

			for ( Vector4D *curve : curves ) {
				for ( size_t i = 0; i < 4; ++i ) {
					curve[i] = align.TransformCoord3Unit( curve[i] + offset ) + mKnots[knot0].mPoint;
				}
			}

			mKnots[knot0].mOutVec = ( curves[0][1] - curves[0][0] ).ToXMVECTOR();

			for ( size_t c = 0; c < 4; ++c ) {
				AddKnot();
				mSegmentType[knot0 + c] = c == 0 ? ESegmentType::FullLoop : ESegmentType::Child;

				mTangentType[knot0 + 1 + c] = ETangentType::Aligned;
				mExtrusions[knot0 + 1 + c].mBitangent = mExtrusions[knot0].mBitangent;
				mExtrusionTypes[knot0 + 1 + c] &= ~CustomNormal;
				mExtrusionTypes[knot0 + 1 + c] |= CustomBitangent;

				mKnots[knot0 + 1 + c].mPoint = curves[c][3];
				mKnots[knot0 + 1 + c].mInVec = ( curves[c][2] - curves[c][3] ).ToXMVECTOR();
				mKnots[knot0 + 1 + c].mOutVec = ( curves[c][3] - curves[c][2] ).ToXMVECTOR();

				UpdateTangentValue( knot0 + 1 + c, false );
				ComputeAutoExtrusions( knot0 + 1 + c, false );

				mExtrusionTypes[knot0 + 1 + c] &= ~CustomBitangent;
				ComputeAutoExtrusions( knot0 + 1 + c, false );
			}
		}

		void AddCurve( double inT = DirectX::XM_PIDIV2, double radius = 5.0f )
		{
			using Cyclone::Math::Vector4D;
			using Cyclone::Math::Matrix44D;

			size_t knot0 = mKnots.size() - 1;

			mExtrusionTypes[knot0] &= ~( CustomNormal | CustomBitangent );
			mTangentType[knot0] = ETangentType::Aligned;

			UpdateTangentValue( knot0, false );
			ComputeAutoExtrusions( knot0 );

			auto c_bezier = [inT, radius]( double u0, double u1, Vector4D *C ) {
				auto c = [inT, radius]( double u ) {
					return Vector4D( radius * std::cos( inT * u ) - radius, 0.0, std::abs( radius ) * std::sin( inT * u ) );
				};

				auto c_prime = [inT, radius]( double u ) {
					return Vector4D( - radius * inT * std::sin( inT * u ), 0.0, inT * std::abs( radius ) * std::cos( inT * u ) );
				};

				C[0] = c( u0 );
				C[3] = c( u1 );

				Vector4D T0 = c_prime( u0 );
				Vector4D T1 = c_prime( u1 );

				Vector4D delta = Vector4D::sReplicate( ( u1 - u0 ) / 3.0 );

				C[1] = C[0] + delta * T0;
				C[2] = C[3] - delta * T1;
			};

			Vector4D C[4] = { Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr }, Vector4D{ nullptr } };

			c_bezier( 0.0, 1.0, C );

			Matrix44D align(
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mExtrusions[knot0].mBitangent, 0 ) ),
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mExtrusions[knot0].mNormal, 0 ) ),
				Vector4D::sFromXMVECTOR( DirectX::XMVectorSetW( mKnots[knot0].mOutVec, 0 ) ).GetNorm3(),
				Vector4D( 0, 0, 0, 1 )
			);

			for ( size_t i = 0; i < 4; ++i ) {
				C[i] = align.TransformCoord3Unit( C[i] ) + mKnots[knot0].mPoint;
			}

			mKnots[knot0].mOutVec = ( C[1] - C[0] ).ToXMVECTOR();

			AddKnot();
			mSegmentType[knot0] = ESegmentType::Curve;

			mTangentType[knot0 + 1] = ETangentType::Aligned;
			mExtrusions[knot0 + 1].mNormal = mExtrusions[knot0].mNormal;
			mExtrusionTypes[knot0 + 1] &= ~CustomBitangent;
			mExtrusionTypes[knot0 + 1] |= CustomNormal;

			mKnots[knot0 + 1].mPoint = C[3];
			mKnots[knot0 + 1].mInVec = ( C[2] - C[3] ).ToXMVECTOR();
			mKnots[knot0 + 1].mOutVec = ( C[3] - C[2] ).ToXMVECTOR();

			UpdateTangentValue( knot0 + 1, false );
			ComputeAutoExtrusions( knot0 + 1, false );

			mExtrusionTypes[knot0 + 1] &= ~CustomNormal;
			ComputeAutoExtrusions( knot0 + 1, true );
		}

		static Cyclone::Math::Vector4D XM_CALLCONV sInterpolate( const Cyclone::Math::Vector4D inP0, const Cyclone::Math::Vector4D inP3, DirectX::FXMVECTOR inH1, DirectX::FXMVECTOR inH2, double inU )
		{
			using Cyclone::Math::Vector4D;

			const Vector4D P1 = inP0 + Vector4D::sFromXMVECTOR( inH1 );
			const Vector4D P2 = inP3 + Vector4D::sFromXMVECTOR( inH2 );

			const double u2 = inU * inU;
			const double u3 = u2 * inU;

			const double iu = 1.0 - inU;
			const double iu2 = iu * iu;
			const double iu3 = iu2 * iu;

			const Vector4D b0 = Vector4D::sReplicate( iu3 );
			const Vector4D b1 = Vector4D::sReplicate( 3 * inU * iu2 );
			const Vector4D b2 = Vector4D::sReplicate( 3 * u2 * iu );
			const Vector4D b3 = Vector4D::sReplicate( u3 );

			Vector4D result = inP0 * b0;
			result = Vector4D::sFusedMultiplyAdd( P1, b1, result );
			result = Vector4D::sFusedMultiplyAdd( P2, b2, result );
			result = Vector4D::sFusedMultiplyAdd( inP3, b3, result );

			return result;
		}

		Cyclone::Math::Vector4D XM_CALLCONV Interpolate( size_t inRoot, double inU ) const
		{
			return sInterpolate( mKnots[inRoot].mPoint, mKnots[inRoot + 1].mPoint, mKnots[inRoot].mOutVec, mKnots[inRoot + 1].mInVec, inU );
		}

		Cyclone::Math::Vector4D XM_CALLCONV Differentiate( size_t inRoot, double inU ) const
		{
			using Cyclone::Math::Vector4D;

			const size_t knot0 = inRoot;
			const size_t knot1 = knot0 + 1;

			const double u2 = inU * inU;

			const double iu = 1.0 - inU;
			const double iu2 = iu * iu;

			const Vector4D b0 = Vector4D::sReplicate( 3 * iu2 );
			const Vector4D b1 = Vector4D::sReplicate( 6 * inU * iu );
			const Vector4D b2 = Vector4D::sReplicate( 3 * u2 );

			const Vector4D outVec = Vector4D::sFromXMVECTOR( mKnots[knot0].mOutVec );
			const Vector4D inVec = Vector4D::sFromXMVECTOR( mKnots[knot1].mInVec );

			Vector4D result = outVec * b0;
			result = Vector4D::sFusedMultiplyAdd( mKnots[knot1].mPoint - mKnots[knot0].mPoint + inVec - outVec, b1, result );
			result = Vector4D::sFusedMultiplyAdd( -inVec, b2, result );

			return result;
		}

		Cyclone::Math::Vector4D XM_CALLCONV Differentiate2( size_t inRoot, double inU ) const
		{
			using Cyclone::Math::Vector4D;

			const size_t knot0 = inRoot;
			const size_t knot1 = knot0 + 1;

			const Vector4D outVec = Vector4D::sFromXMVECTOR( mKnots[knot0].mOutVec );
			const Vector4D inVec = Vector4D::sFromXMVECTOR( mKnots[knot1].mInVec );

			const Vector4D PD = mKnots[knot1].mPoint - mKnots[knot0].mPoint;
			const Vector4D n2 = Vector4D::sReplicate( -2.0 );

			const Vector4D C0 = Vector4D::sFusedMultiplyAdd( outVec, n2, inVec + PD );
			const Vector4D C1 = Vector4D::sFusedMultiplyAdd( inVec, n2, outVec - PD );

			const Vector4D b0 = Vector4D::sReplicate( 6 * ( 1 - inU ) );
			const Vector4D b1 = Vector4D::sReplicate( 6 * inU );

			Vector4D result = C0 * b0;
			result = Vector4D::sFusedMultiplyAdd( C1, b1, result);

			return result;
		}

		Cyclone::Math::Vector4D XM_CALLCONV Differentiate3( size_t inRoot ) const
		{
			using Cyclone::Math::Vector4D;

			const size_t knot0 = inRoot;
			const size_t knot1 = knot0 + 1;

			const Vector4D d12 = Vector4D::sReplicate( 12.0 );
			const Vector4D d18 = Vector4D::sReplicate( 18.0 );

			Vector4D result = mKnots[knot0].mPoint * d12;
			result = Vector4D::sFusedMultiplyAdd( Vector4D::sFromXMVECTOR( mKnots[knot0].mOutVec ), d18, result );
			result = Vector4D::sFusedNegativeMultiplyAdd( Vector4D::sFromXMVECTOR( mKnots[knot1].mInVec ), d18, result );
			result = Vector4D::sFusedNegativeMultiplyAdd( mKnots[knot1].mPoint, d12, result );

			return result;
		}

		Cyclone::Math::Vector4D XM_CALLCONV ComputeKappaVector( size_t inRoot, double inU ) const
		{
			using Cyclone::Math::Vector4D;

			const Vector4D rPrime = Differentiate( inRoot, inU );
			const Vector4D rDPrime = Differentiate2( inRoot, inU );
			const Vector4D tHat = rPrime.GetNorm3();

			return ( rDPrime - Vector4D::sReplicate( rDPrime.Dot3( tHat ) ) * tHat ) / Vector4D::sReplicate( rPrime.Dot3() );
		}

		static double XM_CALLCONV sComputeScale( Cyclone::Math::Vector4D inKappa, Cyclone::Math::Vector4D inDisplacement )
		{
			const Cyclone::Math::Vector4D direction = inDisplacement.GetNorm3();
			const double distance = inDisplacement.GetLength3();

			const double kSigned = -inKappa.Dot3( direction );

			return distance > 0 ? distance * kSigned : 0;
		}

		double GravesenLength( size_t inRoot ) const
		{
			using Cyclone::Math::Vector4D;

			const Vector4D pointDelta = mKnots[inRoot + 1].mPoint - mKnots[inRoot].mPoint;
			const Vector4D outVec = Vector4D::sFromXMVECTOR( mKnots[inRoot].mOutVec );
			const Vector4D inVec = Vector4D::sFromXMVECTOR( mKnots[inRoot].mInVec );

			const double lowerBound = pointDelta.GetLength3();
			const double upperBound = inVec.GetLength3() + ( pointDelta + outVec - inVec ).GetLength3() + outVec.GetLength3();

			return lowerBound * 0.25 + upperBound * 0.75;
		}

		void UpdateTangentType( size_t root, bool priorityOutVec )
		{
			if ( mTangentType[root] != ETangentType::Split ) {
				DirectX::XMVECTOR iLen = DirectX::XMVector3Length( mKnots[root].mInVec );
				DirectX::XMVECTOR oLen = DirectX::XMVector3Length( mKnots[root].mOutVec );

				DirectX::XMVECTOR iNorm = DirectX::XMVectorDivide( mKnots[root].mInVec, iLen );
				DirectX::XMVECTOR oNorm = DirectX::XMVectorDivide( mKnots[root].mOutVec, oLen );


				if ( mTangentType[root] == ETangentType::Mirrored ) {
					DirectX::XMVECTOR aLen = DirectX::XMVectorMultiply( DirectX::XMVectorAdd( iLen, oLen ), DirectX::XMVectorReplicate( 0.5f ) );
					if ( priorityOutVec ) {
						mKnots[root].mInVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( oNorm ), aLen );
						mKnots[root].mOutVec = DirectX::XMVectorMultiply( oNorm, aLen );
					}
					else {
						mKnots[root].mInVec = DirectX::XMVectorMultiply( iNorm, aLen );
						mKnots[root].mOutVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( iNorm ), aLen );
					}
				}
				else if ( mTangentType[root] == ETangentType::Aligned ) {
					if ( priorityOutVec ) {
						mKnots[root].mInVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( oNorm ), iLen );
						mKnots[root].mOutVec = DirectX::XMVectorMultiply( oNorm, oLen );
					}
					else {
						mKnots[root].mInVec = DirectX::XMVectorMultiply( iNorm, iLen );
						mKnots[root].mOutVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( iNorm ), oLen );
					}
				}
			}
		}

		void UpdateTangentValue( size_t root, bool priorityOutVec )
		{
			DirectX::XMVECTOR iLen = DirectX::XMVector3Length( mKnots[root].mInVec );
			DirectX::XMVECTOR oLen = DirectX::XMVector3Length( mKnots[root].mOutVec );

			DirectX::XMVECTOR iNorm = DirectX::XMVectorDivide( mKnots[root].mInVec, iLen );
			DirectX::XMVECTOR oNorm = DirectX::XMVectorDivide( mKnots[root].mOutVec, oLen );

			if ( mTangentType[root] == ETangentType::Mirrored ) {
				if ( priorityOutVec ) {
					mKnots[root].mInVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( oNorm ), oLen );
					mKnots[root].mOutVec = DirectX::XMVectorMultiply( oNorm, oLen );
				}
				else {
					mKnots[root].mInVec = DirectX::XMVectorMultiply( iNorm, iLen );
					mKnots[root].mOutVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( iNorm ), iLen );
				}
			}
			else if ( mTangentType[root] == ETangentType::Aligned ) {
				if ( priorityOutVec ) {
					mKnots[root].mInVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( oNorm ), iLen );
					mKnots[root].mOutVec = DirectX::XMVectorMultiply( oNorm, oLen );
				}
				else {
					mKnots[root].mInVec = DirectX::XMVectorMultiply( iNorm, iLen );
					mKnots[root].mOutVec = DirectX::XMVectorMultiply( DirectX::XMVectorNegate( iNorm ), oLen );
				}
			}
		}

		void ComputeAutoExtrusions( size_t root, bool priorityBitangent = true )
		{
			if ( priorityBitangent ) {
				if ( !( mExtrusionTypes[root] & EExtrusionType::CustomBitangent ) ) {
					mExtrusions[root].mBitangent = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( mExtrusions[root].mNormal, DirectX::XMVector3Normalize( mKnots[root].mOutVec ) ) );
				}
				if ( !( mExtrusionTypes[root] & EExtrusionType::CustomNormal ) ) {
					mExtrusions[root].mNormal = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( DirectX::XMVector3Normalize( mKnots[root].mOutVec ), mExtrusions[root].mBitangent ) );
				}
			}
			else {
				if ( !( mExtrusionTypes[root] & EExtrusionType::CustomNormal ) ) {
					mExtrusions[root].mNormal = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( DirectX::XMVector3Normalize( mKnots[root].mOutVec ), mExtrusions[root].mBitangent ) );
				}
				if ( !( mExtrusionTypes[root] & EExtrusionType::CustomBitangent ) ) {
					mExtrusions[root].mBitangent = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( mExtrusions[root].mNormal, DirectX::XMVector3Normalize( mKnots[root].mOutVec ) ) );
				}
			}
		}

		void ValidatePath()
		{
			for ( size_t i = 0; i < mKnots.size(); ++i ) {
				UpdateTangentValue( i, true );
				ComputeAutoExtrusions( i, true );
			}
		}
	};

	struct PathCache
	{
		struct Interpolation
		{
			Cyclone::Math::Vector4D mPosition;
			DirectX::XMVECTOR		mDeltaL;
			DirectX::XMVECTOR		mDeltaR;
			DirectX::XMVECTOR		mDeltaLU;
			DirectX::XMVECTOR		mDeltaRU;
		};

		static constexpr double		kSubdivisionFactor = 10.0; ///< Subdivisions per meter

		std::vector<Interpolation>	mArray;
		std::vector<uint32_t>		mCumulativeSubdivisions;

		void Rebuild( entt::handle &inHandle )
		{
			using Cyclone::Math::Vector4D;

			const PathData &pathData = inHandle.get<PathData>();

			assert( pathData.mKnots.size() > 0 );

			mCumulativeSubdivisions.resize( pathData.mKnots.size() );
			mCumulativeSubdivisions[0] = 0;

			std::vector<PathData::Knot> sideL = pathData.mKnots;
			std::vector<PathData::Knot> sideR = pathData.mKnots;
			std::vector<PathData::Knot> sideLU = pathData.mKnots;
			std::vector<PathData::Knot> sideRU = pathData.mKnots;

			for ( size_t i = 0; i + 1 < pathData.mKnots.size(); ++i ) {
				mCumulativeSubdivisions[i + 1] = mCumulativeSubdivisions[i] + std::max( 2U, 1 + static_cast<uint32_t>( std::lround( pathData.GravesenLength( i ) ) ) );

				float halfWidth0 = pathData.mPathWidths[i] / 2;
				float halfWidth1 = pathData.mPathWidths[i + 1] / 2;

				const Vector4D corr0B = !( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CurveBitangent ) ? Vector4D::sZero() : Vector4D::sOne();
				const Vector4D corr1B = !( pathData.mExtrusionTypes[i + 1] & PathData::EExtrusionType::CurveBitangent ) ? Vector4D::sZero() : Vector4D::sOne();

				const Vector4D corr0N = !( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CurveNormal ) ? Vector4D::sZero() : Vector4D::sOne();
				const Vector4D corr1N = !( pathData.mExtrusionTypes[i + 1] & PathData::EExtrusionType::CurveNormal ) ? Vector4D::sZero() : Vector4D::sOne();

				const Vector4D dispB0 = Vector4D::sFromXMVECTOR( DirectX::XMVectorScale( pathData.mExtrusions[i].mBitangent, halfWidth0 ) );
				const Vector4D dispB1 = Vector4D::sFromXMVECTOR( DirectX::XMVectorScale( pathData.mExtrusions[i + 1].mBitangent, halfWidth1 ) );

				const Vector4D dispN0 = Vector4D::sFromXMVECTOR( DirectX::XMVectorScale( pathData.mExtrusions[i].mNormal, -0.1f ) );
				const Vector4D dispN1 = Vector4D::sFromXMVECTOR( DirectX::XMVectorScale( pathData.mExtrusions[i + 1].mNormal, -0.1f ) );

				const Vector4D kVec0 = pathData.ComputeKappaVector( i, 0.0 );
				const Vector4D kVec1 = pathData.ComputeKappaVector( i, 1.0 );

				const float scaleL0 = static_cast<float>( 1 + PathData::sComputeScale( kVec0, dispB0 * corr0B ) );
				const float scaleL1 = static_cast<float>( 1 + PathData::sComputeScale( kVec1, dispB1 * corr1B ) );

				const float scaleR0 = static_cast<float>( 1 + PathData::sComputeScale( kVec0, -dispB0 * corr0B ) );
				const float scaleR1 = static_cast<float>( 1 + PathData::sComputeScale( kVec1, -dispB1 * corr1B ) );

				const float scaleLU0 = static_cast<float>( 1 + PathData::sComputeScale( kVec0, dispB0 * corr0B + dispN0 * corr0N ) );
				const float scaleLU1 = static_cast<float>( 1 + PathData::sComputeScale( kVec1, dispB1 * corr1B + dispN1 * corr1N ) );

				const float scaleRU0 = static_cast<float>( 1 + PathData::sComputeScale( kVec0, -dispB0 * corr0B + dispN0 * corr0N ) );
				const float scaleRU1 = static_cast<float>( 1 + PathData::sComputeScale( kVec1, -dispB1 * corr1B + dispN1 * corr1N ) );

				OutputDebugStringA( std::format( "Segment={} | L0={:.2f}, R0={:.2f}, L1={:.2f}, R1={:.2f}\n", i, scaleL0, scaleR0, scaleL1, scaleR1 ).c_str() );

				if ( i == 0 ) {
					sideL[i].mPoint += dispB0;
					sideR[i].mPoint += -dispB0;
					sideLU[i].mPoint += dispB0 + dispN0;
					sideRU[i].mPoint += -dispB0 + dispN0;
				}

				sideL[i + 1].mPoint += dispB1;
				sideR[i + 1].mPoint += -dispB1;
				sideLU[i + 1].mPoint += dispB1 + dispN1;
				sideRU[i + 1].mPoint += -dispB1 + dispN1;

				sideL[i].mOutVec = DirectX::XMVectorScale( pathData.mKnots[i].mOutVec, scaleL0 );
				sideL[i + 1].mInVec = DirectX::XMVectorScale( pathData.mKnots[i + 1].mInVec, scaleL1 );

				sideR[i].mOutVec = DirectX::XMVectorScale( pathData.mKnots[i].mOutVec, scaleR0 );
				sideR[i + 1].mInVec = DirectX::XMVectorScale( pathData.mKnots[i + 1].mInVec, scaleR1 );

				sideLU[i].mOutVec = DirectX::XMVectorScale( pathData.mKnots[i].mOutVec, scaleLU0 );
				sideLU[i + 1].mInVec = DirectX::XMVectorScale( pathData.mKnots[i + 1].mInVec, scaleLU1 );

				sideRU[i].mOutVec = DirectX::XMVectorScale( pathData.mKnots[i].mOutVec, scaleRU0 );
				sideRU[i + 1].mInVec = DirectX::XMVectorScale( pathData.mKnots[i + 1].mInVec, scaleRU1 );
			}

			for ( size_t i = 0; i < pathData.mKnots.size(); ++i ) {
				if ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::EaseInOut ) != PathData::EExtrusionType::EaseInOut ) {
					//Vector4D inPlane = -Vector4D::sCross3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent ), Vector4D::sFromXMVECTOR( pathData.mKnots[i].mInVec ).GetNorm3() ).GetNorm3();
					//Vector4D outPlane = Vector4D::sCross3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent ), Vector4D::sFromXMVECTOR( pathData.mKnots[i].mOutVec ).GetNorm3() ).GetNorm3();
					for ( auto pside : { &sideL, &sideR, &sideLU, &sideRU } ) {
						std::vector<PathData::Knot> &side = *pside;

						Vector4D inDelta = i == 0 ? ( side[i + 1].mPoint - side[i].mPoint ) : ( side[i].mPoint - side[i - 1].mPoint );
						Vector4D outDelta = i + 1 == pathData.mKnots.size() ? ( side[i].mPoint - side[i - 1].mPoint ) : ( side[i + 1].mPoint - side[i].mPoint );

						// NOTE
						// SPECIAL INTERP MODE
						//
						if ( false ) {
							inDelta -= Vector4D::sReplicate( inDelta.Dot3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mNormal ) ) ) * Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mNormal );
							outDelta -= Vector4D::sReplicate( outDelta.Dot3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mNormal ) ) ) * Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mNormal );
						}
						//
						// SPECIAL INTERP MODE
						// NOTE

						Vector4D inPlane = Vector4D::sCross3( inDelta.GetNorm3(), Vector4D::sCross3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent ), inDelta.GetNorm3() ).GetNorm3() ).GetNorm3();
						Vector4D outPlane = -Vector4D::sCross3( outDelta.GetNorm3(), Vector4D::sCross3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent ), outDelta.GetNorm3() ).GetNorm3() ).GetNorm3();

						//inDelta = inDelta - Vector4D::sReplicate( inDelta.Dot3( inPlane ) ) * inPlane;
						//outDelta = outDelta - Vector4D::sReplicate( outDelta.Dot3( outPlane ) ) * outPlane;

						//inDelta -= Vector4D::sReplicate( inDelta.Dot3( inPlane ) ) * inPlane;
						//outDelta -= Vector4D::sReplicate( outDelta.Dot3( outPlane ) ) * outPlane;

						Vector4D inDir = inDelta.GetNorm3();
						Vector4D outDir = outDelta.GetNorm3();

						Vector4D inVec = Vector4D::sFromXMVECTOR( side[i].mInVec );
						Vector4D outVec = Vector4D::sFromXMVECTOR( side[i].mOutVec );

						inVec -= Vector4D::sReplicate( inVec.Dot3( inPlane ) ) * inPlane;
						outVec -= Vector4D::sReplicate( outVec.Dot3( outPlane ) ) * outPlane;

						//side[i].mInVec = ( Vector4D::sReplicate( inVec.Dot3( inDir ) ) * inDir ).ToXMVECTOR();
						//side[i].mOutVec = ( Vector4D::sReplicate( outVec.Dot3( outDir ) ) * outDir ).ToXMVECTOR();

						if ( !( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::EaseIn ) ) side[i].mInVec = ( inVec ).ToXMVECTOR();
						if ( !( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::EaseOut ) ) side[i].mOutVec = ( outVec ).ToXMVECTOR();

						//if ( i == 12 ) {
						//	__debugbreak();
						//}
					}
				}
			}

			mArray.clear();
			mArray.reserve( mCumulativeSubdivisions.back() );

			uint32_t prevSubdivisions = mCumulativeSubdivisions.front();

			for ( size_t i = 0; i + 1 < pathData.mKnots.size(); ++i ) {
				uint32_t cumSubdivisions = mCumulativeSubdivisions[i + 1];
				uint32_t numSubdivisions = cumSubdivisions - prevSubdivisions;
				prevSubdivisions = cumSubdivisions;

				for ( size_t t = 0; t < numSubdivisions; ++t ) {
					double u = static_cast<double>( t ) / ( numSubdivisions - 1 );

					Vector4D left = PathData::sInterpolate( sideL[i].mPoint, sideL[i + 1].mPoint, sideL[i].mOutVec, sideL[i + 1].mInVec, u );
					Vector4D right = PathData::sInterpolate( sideR[i].mPoint, sideR[i + 1].mPoint, sideR[i].mOutVec, sideR[i + 1].mInVec, u );
					Vector4D leftU = PathData::sInterpolate( sideLU[i].mPoint, sideLU[i + 1].mPoint, sideLU[i].mOutVec, sideLU[i + 1].mInVec, u );
					Vector4D rightU = PathData::sInterpolate( sideRU[i].mPoint, sideRU[i + 1].mPoint, sideRU[i].mOutVec, sideRU[i + 1].mInVec, u );

					Vector4D p = pathData.Interpolate( i, u );

					mArray.emplace_back( p, ( left - p ).ToXMVECTOR(), ( right - p ).ToXMVECTOR(), ( leftU - p ).ToXMVECTOR(), ( rightU - p ).ToXMVECTOR() );

					//float ldlu = std::abs( DirectX::XMVectorGetX( DirectX::XMVector3Dot( DirectX::XMVector3Normalize( mArray.back().mDeltaL ), DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( mArray.back().mDeltaL, mArray.back().mDeltaLU ) ) ) ) );
					//float rdru = std::abs( DirectX::XMVectorGetX( DirectX::XMVector3Dot( DirectX::XMVector3Normalize( mArray.back().mDeltaR ), DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( mArray.back().mDeltaR, mArray.back().mDeltaRU ) ) ) ) );
					//
					//assert( ldlu < 1e-2f );
					//assert( rdru < 1e-2f );
				}
			}
		}
	};

	struct PathSelection
	{
		uint16_t mCurrentKnot = static_cast<uint16_t>( -1 );
		std::set<uint16_t> mSelectedKnots;

		bool SetSelectedKnot( uint16_t inKnot )
		{
			if ( mCurrentKnot != inKnot || mSelectedKnots.size() != 1 || *mSelectedKnots.begin() != inKnot ) {
				mCurrentKnot = inKnot;
				mSelectedKnots.clear();
				mSelectedKnots.insert( inKnot );
				return true;
			}
			return false;
		}

		bool AddSelectedKnot( uint16_t inKnot )
		{
			if ( mCurrentKnot != inKnot || !mSelectedKnots.contains( inKnot ) ) {
				mCurrentKnot = inKnot;
				mSelectedKnots.insert( inKnot );
				return true;
			}
			return false;
		}

		bool DeselectKnot( uint16_t inKnot )
		{
			if ( mSelectedKnots.erase( inKnot ) ) {
				if ( inKnot == mCurrentKnot || true ) {
					if ( mSelectedKnots.empty() ) mCurrentKnot = static_cast<uint16_t>( -1 );
					else {
						auto it = mSelectedKnots.upper_bound( inKnot );
						if ( it == mSelectedKnots.end() ) {
							it = mSelectedKnots.lower_bound( inKnot );
						}
						if ( it == mSelectedKnots.end() ) {
							it = mSelectedKnots.begin();
						}
						mCurrentKnot = *it;
					}
				}
				return true;
			}
			return false;
		}

		bool ClearSelection()
		{
			if ( mSelectedKnots.size() ) {
				mSelectedKnots.clear();
				mCurrentKnot = static_cast<uint16_t>( -1 );
				return true;
			}
			return false;
		}
	};

	struct PathDependency
	{
		entt::entity mPathEntity{ entt::null };
		uint16_t	 mStartKnot = static_cast<uint16_t>( -1 );
		uint16_t	 mEndKnot = static_cast<uint16_t>( -1 );

		bool Reset()
		{
			bool dirty = false;
			dirty |= mPathEntity != entt::null;
			dirty |= mStartKnot != static_cast<uint16_t>( -1 );
			dirty |= mEndKnot != static_cast<uint16_t>( -1 );

			mPathEntity = entt::null;
			mStartKnot = static_cast<uint16_t>( -1 );
			mEndKnot = static_cast<uint16_t>( -1 );

			return dirty;
		}
	};

	struct PathChildren
	{
		std::set<entt::entity> mChildren;

		void FindChildren( entt::handle &inHandle )
		{
			mChildren.clear();

			auto view = inHandle.registry()->view<PathDependency>();
			for ( entt::entity entity : view ) {
				auto &pathDependency = view.get<PathDependency>( entity );

				if ( pathDependency.mPathEntity == inHandle.entity() ) {
					mChildren.insert( entity );
				}
			}
		}

		bool AddChild( entt::entity inChild )
		{
			return mChildren.insert( inChild ).second;
		}

		bool RemoveChild( entt::entity inChild )
		{
			return mChildren.erase( inChild );
		}
	};
}