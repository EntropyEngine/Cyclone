#pragma once

#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Path.hpp"

namespace Cyclone::Core::Component
{
	struct LocalBounds
	{
		enum class EType { Radius, BoundingBox, Path };

		DirectX::XMVECTOR mCenter;
		DirectX::XMFLOAT3 mExtent;
		EType mType;

		void UpdateCenterExtent( entt::entity inEntity, entt::registry &inRegistry )
		{
			switch ( mType ) {
				case EType::Radius: {
					return;
				}
				case EType::BoundingBox: {
					return;
				}
				case EType::Path: {
					const PathData &pathData = inRegistry.get<PathData>( inEntity );

					Cyclone::Math::Vector4D bbMin = Cyclone::Math::Vector4D::sPosInf();
					Cyclone::Math::Vector4D bbMax = Cyclone::Math::Vector4D::sNegInf();

					for ( const auto &segment : pathData.mPathSegments ) {
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, segment.mP0 );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, segment.mP0 );

						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, segment.mP1 );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, segment.mP1 );

						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, segment.mP2 );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, segment.mP2 );

						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, segment.mP3 );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, segment.mP3 );
					}

					Cyclone::Math::Vector4D half = Cyclone::Math::Vector4D::sReplicate( 0.5 );
					Cyclone::Math::Vector4D bbCenter = ( bbMax + bbMin ) * half;
					Cyclone::Math::Vector4D bbExtent = ( bbMax - bbMin ) * half;

					mCenter = bbCenter.ToXMVECTOR();
					DirectX::XMStoreFloat3( &mExtent, bbExtent.ToXMVECTOR() );

					return;
				}
				default:
					assert( false );
					__assume( false );
			}
		}

		void UpdateBoundingBox( entt::entity inEntity, entt::registry &inRegistry ) const
		{
			switch ( mType ) {
				case EType::Radius: {
					const Rotation &rotation = inRegistry.get<Rotation>( inEntity );
					DirectX::XMMATRIX rotmat = DirectX::XMMatrixRotationRollPitchYawFromVector( rotation.mPitchYawRoll );
					DirectX::XMVECTOR newcenter = DirectX::XMVector3TransformCoord( mCenter, rotmat );
					Cyclone::Math::Vector4D newextent = Cyclone::Math::Vector4D::sReplicate( mExtent.x );
					inRegistry.patch<BoundingBox>( inEntity, [newcenter, newextent]( auto &inV ) {
						inV.mValue.mCenter = Cyclone::Math::Vector4D::sFromXMVECTOR( newcenter );
						inV.mValue.mExtent = newextent;
					} );
					return;
				}
				case EType::BoundingBox:
				case EType::Path: {
					const Rotation &rotation = inRegistry.get<Rotation>( inEntity );
					
					DirectX::XMMATRIX rotmat = DirectX::XMMatrixRotationRollPitchYawFromVector( rotation.mPitchYawRoll );
					DirectX::XMVECTOR newcenter = DirectX::XMVector3TransformCoord( mCenter, rotmat );
					DirectX::XMVECTOR newextent = DirectX::XMVectorAdd(
						DirectX::XMVectorAdd(
							DirectX::XMVectorMultiply( DirectX::XMVectorAbs( rotmat.r[0] ), DirectX::XMVectorReplicate( mExtent.x ) ),
							DirectX::XMVectorMultiply( DirectX::XMVectorAbs( rotmat.r[1] ), DirectX::XMVectorReplicate( mExtent.y ) )
						),
						DirectX::XMVectorMultiply( DirectX::XMVectorAbs( rotmat.r[2] ), DirectX::XMVectorReplicate( mExtent.z ) )
					);
					inRegistry.patch<BoundingBox>( inEntity, [newcenter, newextent]( auto &inV ) {
						inV.mValue.mCenter = Cyclone::Math::Vector4D::sFromXMVECTOR( newcenter );
						inV.mValue.mExtent = Cyclone::Math::Vector4D::sFromXMVECTOR( newextent );
					} );
					return;
				}
				default:
					assert( false );
					__assume( false );
			}
		}
	};
}