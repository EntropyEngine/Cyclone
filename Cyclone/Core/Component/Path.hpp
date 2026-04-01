#pragma once

#include "Cyclone/Math/Vector.hpp"

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

		enum EExtrusionType : uint8_t
		{
			NormalExplicit		= ( 1 << 0 ),
			NormalAligned		= ( 2 << 0 ),
			NormalTilt			= ( 3 << 0 ),

			BitangentExplicit	= ( 1 << 2 ),
			BitangentAligned	= ( 2 << 2 ),
			BitangentTilt		= ( 3 << 2 ),

			Explicit			= NormalExplicit | BitangentExplicit,
			Aligned				= NormalAligned | BitangentAligned,
			Tilt				= NormalTilt | BitangentTilt,

			TYPE_MASK			= ( 0b11 << 0 ),
			TYPE_SHIFT			= ( 1 << 2 ),
			NORMAL_MASK			= TYPE_MASK,
			BITANGENT_MASK		= TYPE_MASK << 2,
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

		DirectX::XMVECTOR XM_CALLCONV InterpolateBitangent( size_t root, float u ) const
		{
			return DirectX::XMVectorLerp( mExtrusions[root].mBitangent, mExtrusions[root + 1].mBitangent, u );
		}

		DirectX::XMVECTOR XM_CALLCONV InterpolateNormal( size_t root, float u ) const
		{
			return DirectX::XMVectorLerp( mExtrusions[root].mNormal, mExtrusions[root + 1].mNormal, u );
		}

		Cyclone::Math::Vector4D XM_CALLCONV InterpolateUVW( size_t root, float u, float v, float w ) const
		{
			using Cyclone::Math::Vector4D;

			Vector4D p = Interpolate( root, u );

			Vector4D t = Vector4D::sFromXMVECTOR( Differentiate( root, u ) ).GetNorm3();

			Vector4D normalExpl = Vector4D::sFromXMVECTOR( InterpolateNormal( root, u ) ).GetNorm3(); // NormalExplict
			Vector4D bitangentExpl = Vector4D::sFromXMVECTOR( InterpolateBitangent( root, u ) ).GetNorm3(); // BitangentExplicit

			Vector4D normalImpl = Vector4D::sCross3( t, bitangentExpl ).GetNorm3(); // NormalTilt
			Vector4D bitangentImpl = -Vector4D::sCross3( t, normalExpl ).GetNorm3(); // BitangentAligned

			Vector4D normalImpl2 = Vector4D::sCross3( t, bitangentImpl ).GetNorm3(); // NormalAligned
			Vector4D bitangentImpl2 = -Vector4D::sCross3( t, normalImpl ).GetNorm3(); // BitangentTilt

			assert( std::abs( normalExpl.Dot3( bitangentExpl ) ) < 0.1e-7 );
			assert( std::abs( normalImpl.Dot3( bitangentImpl2 ) ) < 0.1e-7 );
			assert( std::abs( normalImpl2.Dot3( bitangentImpl ) ) < 0.1e-7 );

			Vector4D normal{ nullptr };
			Vector4D bitangent{ nullptr };

			switch ( mExtrusionTypes[root] & NORMAL_MASK ) {
				case NormalExplicit: normal = normalExpl; break;
				case NormalAligned:  normal = normalImpl2; break;
				case NormalTilt:	 normal = normalImpl; break;
				default:
					assert( false );
					__assume( false );
			}

			switch ( mExtrusionTypes[root] & BITANGENT_MASK ) {
				case BitangentExplicit:	bitangent = bitangentExpl; break;
				case BitangentAligned:	bitangent = bitangentImpl; break;
				case BitangentTilt:		bitangent = bitangentImpl2; break;
				default:
					assert( false );
					__assume( false );
			}

			assert( std::abs( normal.Dot3( bitangent ) ) < 0.1e-7 );
			if ( mExtrusionTypes[root] != Explicit ) {
				assert( std::abs( normal.Dot3( t ) ) < 0.1e-7 );
			}

			return p + bitangent * Vector4D::sReplicate( v ) + normal * Vector4D::sReplicate( w );
		}
	};
}