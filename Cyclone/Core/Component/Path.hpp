#pragma once

#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/Math/Matrix.hpp"

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
			NormalExplicit		= ( 1 << 0 ),
			NormalTwist		= ( 2 << 0 ),
			NormalTilt			= ( 3 << 0 ),

			BitangentExplicit	= ( 1 << 2 ),
			BitangentTwist	= ( 2 << 2 ),
			BitangentTilt		= ( 3 << 2 ),

			Explicit			= NormalExplicit | BitangentExplicit,
			Twist				= NormalTwist | BitangentTwist,
			Tilt				= NormalTilt | BitangentTilt,

			TYPE_MASK			= ( 0b11 << 0 ),
			TYPE_SHIFT			= 2,
			NORMAL_MASK			= TYPE_MASK,
			BITANGENT_MASK		= TYPE_MASK << 2,

			CustomNormal		= ( 0b01 << ( TYPE_SHIFT + TYPE_SHIFT ) ),
			CustomBitangent		= ( 0b10 << ( TYPE_SHIFT + TYPE_SHIFT ) )
		};

		static constexpr const char * kTangentTypes[] = { "Split", "Aligned", "Mirrored" };
		enum class ETangentType : uint8_t
		{
			Split,
			Aligned,
			Mirrored
		};

		enum class ESegmentType : uint8_t
		{

		};

		struct Segment
		{
			Cyclone::Math::Vector4D mP0;
			Cyclone::Math::Vector4D mP1;
			Cyclone::Math::Vector4D mP2;
			Cyclone::Math::Vector4D mP3;

			Cyclone::Math::Vector4D XM_CALLCONV GetPoint( double u ) const
			{
				const double u2 = u * u;
				const double u3 = u2 * u;

				const double iu = 1.0 - u;
				const double iu2 = iu * iu;
				const double iu3 = iu2 * iu;

				const Cyclone::Math::Vector4D b0 = Cyclone::Math::Vector4D::sReplicate( iu3 );
				const Cyclone::Math::Vector4D b1 = Cyclone::Math::Vector4D::sReplicate( 3.0 * u * iu2 );
				const Cyclone::Math::Vector4D b2 = Cyclone::Math::Vector4D::sReplicate( 3.0 * u2 * iu );
				const Cyclone::Math::Vector4D b3 = Cyclone::Math::Vector4D::sReplicate( u3 );

				return mP0 * b0 + mP1 * b1 + mP2 * b2 + mP3 * b3;
			}

			Cyclone::Math::Vector4D XM_CALLCONV GetDerivative( double u ) const
			{
				const double u2 = u * u;

				const double iu = 1.0 - u;
				const double iu2 = iu * iu;

				const Cyclone::Math::Vector4D b0 = Cyclone::Math::Vector4D::sReplicate( 3.0 * iu2 );
				const Cyclone::Math::Vector4D b1 = Cyclone::Math::Vector4D::sReplicate( 6.0 * u * iu );
				const Cyclone::Math::Vector4D b2 = Cyclone::Math::Vector4D::sReplicate( 3.0 * u2 );

				return ( mP1 - mP0 ) * b0 + ( mP2 - mP1 ) * b1 + ( mP3 - mP2 ) * b2;
			}
		};

		std::vector<Knot>			mKnots;
		std::vector<Extrusion>		mExtrusions;
		std::vector<uint8_t>		mExtrusionTypes;
		std::vector<ETangentType>	mTangentType;

		void AddKnot()
		{
			if ( mKnots.size() == 0 ) {
				mKnots.emplace_back( Cyclone::Math::Vector4D::sZero(), DirectX::XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f ), DirectX::XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f ) );
				mExtrusions.emplace_back( DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ), DirectX::XMVectorSet( 1.0f, 0.0f, 0.0f, 0.0f ) );
				mExtrusionTypes.push_back( EExtrusionType::Tilt );
				mTangentType.push_back( ETangentType::Aligned );
			}
			else {
				size_t i = mKnots.size() - 1;
				mKnots.emplace_back( mKnots[i].mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( mKnots[i].mOutVec ) * Cyclone::Math::Vector4D::sReplicate( 3.0f ), DirectX::XMVectorNegate( mKnots[i].mOutVec ), mKnots[i].mOutVec);
				mExtrusions.emplace_back( mExtrusions[i].mNormal, mExtrusions[i].mBitangent );
				mExtrusionTypes.push_back( EExtrusionType::Tilt );
				mTangentType.push_back( ETangentType::Aligned );
			}
		}

		void AddLoop( double inT = DirectX::XM_2PI, double inStride = 2.0f, double inRadius = 2.0f, bool inFull = true )
		{
			using Cyclone::Math::Vector4D;
			using Cyclone::Math::Matrix44D;

			size_t knot0 = mKnots.size() - 1;

			inT /= inFull ? 2 : 1;
			inStride /= inFull ? 2 : 1;
			inStride /= inRadius;

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

			mExtrusionTypes[knot0] &= ~( CustomNormal | CustomBitangent );
			mTangentType[knot0] = ETangentType::Aligned;

			UpdateTangentValue( knot0, false );
			ComputeAutoExtrusions( knot0 );

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
			ComputeAutoExtrusions( knot0 + 1 );
			ComputeAutoExtrusions( knot0 + 2 );

		}

		Cyclone::Math::Vector4D XM_CALLCONV Interpolate( size_t root, float u ) const
		{
			const size_t knot0 = root;
			const size_t knot1 = knot0 + 1;

			using namespace DirectX;

			DirectX::XMVECTOR P0 = g_XMZero;
			DirectX::XMVECTOR P1 = mKnots[knot0].mOutVec;
			DirectX::XMVECTOR P3 = ( mKnots[knot1].mPoint - mKnots[knot0].mPoint ).ToXMVECTOR();
			DirectX::XMVECTOR P2 = P3 + mKnots[knot1].mInVec;

			const float u2 = u * u;
			const float u3 = u2 * u;

			const float iu = 1.0f - u;
			const float iu2 = iu * iu;
			const float iu3 = iu2 * iu;

			DirectX::XMVECTOR b0 = DirectX::XMVectorReplicate( iu3 );
			DirectX::XMVECTOR b1 = DirectX::XMVectorReplicate( 3 * u * iu2 );
			DirectX::XMVECTOR b2 = DirectX::XMVectorReplicate( 3 * u2 * iu );
			DirectX::XMVECTOR b3 = DirectX::XMVectorReplicate( u3 );

			DirectX::XMVECTOR result = DirectX::XMVectorMultiply( P0, b0 );
			result = DirectX::XMVectorMultiplyAdd( P1, b1, result );
			result = DirectX::XMVectorMultiplyAdd( P2, b2, result );
			result = DirectX::XMVectorMultiplyAdd( P3, b3, result );

			return Cyclone::Math::Vector4D::sFromXMVECTOR( result ) + mKnots[knot0].mPoint;
		}

		DirectX::XMVECTOR XM_CALLCONV Differentiate( size_t root, float u ) const
		{
			const size_t knot0 = root;
			const size_t knot1 = knot0 + 1;

			using namespace DirectX;

			const float u2 = u * u;

			const float iu = 1.0f - u;
			const float iu2 = iu * iu;

			DirectX::XMVECTOR b0 = DirectX::XMVectorReplicate( 3.0f * iu2 );
			DirectX::XMVECTOR b1 = DirectX::XMVectorReplicate( 6 * u * iu );
			DirectX::XMVECTOR b2 = DirectX::XMVectorReplicate( 3 * u2 );

			DirectX::XMVECTOR result = DirectX::XMVectorMultiply( mKnots[knot0].mOutVec, b0 );
			result = DirectX::XMVectorMultiplyAdd( ( mKnots[knot1].mPoint - mKnots[knot0].mPoint ).ToXMVECTOR() + mKnots[knot1].mInVec - mKnots[knot0].mOutVec, b1, result);
			result = DirectX::XMVectorMultiplyAdd( -mKnots[knot1].mInVec, b2, result );

			return result;
		}

		DirectX::XMVECTOR XM_CALLCONV Differentiate2( size_t root, float u ) const
		{
			const size_t knot0 = root;
			const size_t knot1 = knot0 + 1;

			using namespace DirectX;

			DirectX::XMVECTOR PD = ( mKnots[knot1].mPoint - mKnots[knot0].mPoint ).ToXMVECTOR();
			DirectX::XMVECTOR n2 = DirectX::XMVectorReplicate( -2.0f );

			DirectX::XMVECTOR C0 = DirectX::XMVectorMultiplyAdd( mKnots[knot0].mOutVec, n2, mKnots[knot1].mInVec + PD );
			DirectX::XMVECTOR C1 = DirectX::XMVectorMultiplyAdd( mKnots[knot1].mInVec, n2, mKnots[knot0].mOutVec - PD );

			DirectX::XMVECTOR b0 = DirectX::XMVectorReplicate( 6 * ( 1 - u ) );
			DirectX::XMVECTOR b1 = DirectX::XMVectorReplicate( 6 * u );

			DirectX::XMVECTOR result = DirectX::XMVectorMultiply( C0, b0 );
			result = DirectX::XMVectorMultiplyAdd( C1, b1, result);

			return result;
		}

		DirectX::XMVECTOR XM_CALLCONV InterpolateBitangent( size_t root, float u ) const
		{
			return DirectX::XMVectorLerp( mExtrusions[root].mBitangent, mExtrusions[root + 1].mBitangent, u );
		}

		DirectX::XMVECTOR XM_CALLCONV InterpolateNormal( size_t root, float u ) const
		{
			using namespace DirectX;

			//DirectX::XMVECTOR tang1 = Differentiate( root, 0 );
			//DirectX::XMVECTOR quat1 = DirectX::XMVector3Cross( tang1, mExtrusions[root].mNormal );
			//quat1 = DirectX::XMVectorSetW( quat1, DirectX::XMVectorGetX( DirectX::XMVector3Length( tang1 ) * DirectX::XMVector3Length( mExtrusions[root].mNormal ) * DirectX::XMVector3Dot( tang1, mExtrusions[root].mNormal ) ) );
			//quat1 = DirectX::XMQuaternionNormalize( quat1 );
			//
			//DirectX::XMVECTOR tang2 = Differentiate( root, 1 );
			//DirectX::XMVECTOR quat2 = DirectX::XMVector3Cross( tang2, mExtrusions[root + 1].mNormal );
			//quat2 = DirectX::XMVectorSetW( quat2, DirectX::XMVectorGetX( DirectX::XMVector3Length( tang2 ) * DirectX::XMVector3Length( mExtrusions[root + 1].mNormal ) * DirectX::XMVector3Dot( tang2, mExtrusions[root + 1].mNormal ) ) );
			//quat2 = DirectX::XMQuaternionNormalize( quat2 );
			//
			//DirectX::XMVECTOR quats = DirectX::XMQuaternionSlerp( quat1, quat2, u );
			//
			//return DirectX::XMVector3TransformCoord( tang1, DirectX::XMMatrixRotationQuaternion( quats ) );

			DirectX::XMVECTOR quat1 = DirectX::XMVector3Cross( mExtrusions[root + 1].mNormal, mExtrusions[root].mNormal );
			quat1 = DirectX::XMVectorSetW( quat1, DirectX::XMVectorGetX( DirectX::XMVector3Length( mExtrusions[root].mNormal ) * DirectX::XMVector3Length( mExtrusions[root + 1].mNormal ) * DirectX::XMVector3Dot( mExtrusions[root].mNormal, mExtrusions[root + 1].mNormal ) ) );
			quat1 = DirectX::XMQuaternionNormalize( quat1 );

			DirectX::XMVECTOR quats = DirectX::XMQuaternionNormalize( DirectX::XMQuaternionSlerp( DirectX::XMQuaternionIdentity(), quat1, u / 2 ) );

			return DirectX::XMQuaternionMultiply( DirectX::XMQuaternionMultiply( quats, mExtrusions[root].mNormal ), DirectX::XMQuaternionInverse( quats ) );
		}

		Cyclone::Math::Vector4D XM_CALLCONV InterpolateUVW( size_t root, float u, float v, float w ) const
		{
			using Cyclone::Math::Vector4D;

			Vector4D p = Interpolate( root, u );

			Vector4D t = Vector4D::sFromXMVECTOR( Differentiate( root, u ) ).GetNorm3();

			Vector4D normalExpl = Vector4D::sFromXMVECTOR( InterpolateNormal( root, u ) ).GetNorm3(); // NormalExplict
			Vector4D bitangentExpl = Vector4D::sFromXMVECTOR( InterpolateBitangent( root, u ) ).GetNorm3(); // BitangentExplicit

			Vector4D normalImpl = Vector4D::sCross3( t, bitangentExpl ).GetNorm3(); // NormalTilt
			Vector4D bitangentImpl = -Vector4D::sCross3( t, normalExpl ).GetNorm3(); // BitangentTwist

			if ( std::abs( t.Dot3( normalExpl ) ) > 1.0f - 0.1e-7 ) {
				bitangentImpl = bitangentExpl;
			}

			if ( std::abs( t.Dot3( bitangentExpl ) ) > 1.0f - 0.1e-7 ) {
				normalImpl = normalExpl;
			}

			Vector4D normalImpl2 = Vector4D::sCross3( t, bitangentImpl ).GetNorm3(); // NormalTwist
			Vector4D bitangentImpl2 = -Vector4D::sCross3( t, normalImpl ).GetNorm3(); // BitangentTilt

			//assert( std::abs( normalExpl.Dot3( bitangentExpl ) ) < 0.1e-7 );
			assert( std::abs( normalImpl.Dot3( bitangentImpl2 ) ) < 0.1e-7 );
			assert( std::abs( normalImpl2.Dot3( bitangentImpl ) ) < 0.1e-7 );

			Vector4D normal{ nullptr };
			Vector4D bitangent{ nullptr };

			switch ( mExtrusionTypes[root] & NORMAL_MASK ) {
				case NormalExplicit: normal = normalExpl; break;
				case NormalTwist:  normal = normalImpl2; break;
				case NormalTilt:	 normal = normalImpl; break;
				default:
					assert( false );
					__assume( false );
			}

			switch ( mExtrusionTypes[root] & BITANGENT_MASK ) {
				case BitangentExplicit:	bitangent = bitangentExpl; break;
				case BitangentTwist:	bitangent = bitangentImpl; break;
				case BitangentTilt:		bitangent = bitangentImpl2; break;
				default:
					assert( false );
					__assume( false );
			}

			//assert( std::abs( normal.Dot3( bitangent ) ) < 0.1e-7 );
			//if ( mExtrusionTypes[root] != Explicit ) {
			//	assert( std::abs( normal.Dot3( t ) ) < 0.1e-7 );
			//}

			return p + bitangent * Vector4D::sReplicate( v ) + normal * Vector4D::sReplicate( w );
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
}