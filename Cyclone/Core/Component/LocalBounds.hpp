#pragma once

#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Path.hpp"

#include "Cyclone/Math/Matrix.hpp"

namespace Cyclone::Core::Component
{
	struct LocalBounds
	{
		enum class EType { Radius, BoundingBox, Path };

		DirectX::XMVECTOR mCenter;
		DirectX::XMFLOAT3 mExtent;
		EType mType;

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
				case EType::BoundingBox: {
					return;
				}
				case EType::Path: {
					const Rotation &rotation = inRegistry.get<Rotation>( inEntity );

					const PathData &pathData = inRegistry.get<PathData>( inEntity );

					Cyclone::Math::Vector4D bbMin = Cyclone::Math::Vector4D::sPosInf();
					Cyclone::Math::Vector4D bbMax = Cyclone::Math::Vector4D::sNegInf();

					Cyclone::Math::Matrix44D rotmat = Cyclone::Math::Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( rotation.mPitchYawRoll ) );

					for ( const auto &segment : pathData.mPathGuide ) {
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, rotmat.TransformCoord3Unit( segment.mP0 ) );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, rotmat.TransformCoord3Unit( segment.mP0 ) );

						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, rotmat.TransformCoord3Unit( segment.mP1 ) );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, rotmat.TransformCoord3Unit( segment.mP1 ) );

						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, rotmat.TransformCoord3Unit( segment.mP2 ) );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, rotmat.TransformCoord3Unit( segment.mP2 ) );

						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, rotmat.TransformCoord3Unit( segment.mP3 ) );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, rotmat.TransformCoord3Unit( segment.mP3 ) );
					}
					
					Cyclone::Math::Vector4D half = Cyclone::Math::Vector4D::sReplicate( 0.5 );
					Cyclone::Math::Vector4D bbCenter = ( bbMax + bbMin ) * half;
					Cyclone::Math::Vector4D bbExtent = ( bbMax - bbMin ) * half;
					
					inRegistry.patch<BoundingBox>( inEntity, [bbCenter, bbExtent]( auto &inV ) {
						inV.mValue.mCenter = bbCenter;
						inV.mValue.mExtent = bbExtent;
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