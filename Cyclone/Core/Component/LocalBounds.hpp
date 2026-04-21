#pragma once

#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Path.hpp"

#include "Cyclone/Math/Matrix.hpp"

namespace Cyclone::Core::Component
{
	struct LocalBounds
	{
		enum class EType { Radius, BoundingBox, Path, LocalMesh, GlobalMesh };

		DirectX::XMVECTOR mCenter;
		DirectX::XMFLOAT3 mExtent;
		EType mType;

		void UpdateBoundingBox( entt::handle &inHandle ) const
		{
			switch ( mType ) {
				case EType::Radius: {
					const Rotation &rotation = inHandle.get<Rotation>();
					DirectX::XMMATRIX rotmat = DirectX::XMMatrixRotationRollPitchYawFromVector( rotation.mPitchYawRoll );
					DirectX::XMVECTOR newcenter = DirectX::XMVector3TransformCoord( mCenter, rotmat );
					Cyclone::Math::Vector4D newextent = Cyclone::Math::Vector4D::sReplicate( mExtent.x );
					inHandle.patch<BoundingBox>( [newcenter, newextent]( auto &inV ) {
						inV.mValue.mCenter = Cyclone::Math::Vector4D::sFromXMVECTOR( newcenter );
						inV.mValue.mExtent = newextent;
					} );
					return;
				}
				case EType::BoundingBox: {
					return;
				}
				case EType::Path: {
					const Rotation &rotation = inHandle.get<Rotation>();

					const PathData &pathData = inHandle.get<PathData>();

					Cyclone::Math::Vector4D bbMin = Cyclone::Math::Vector4D::sPosInf();
					Cyclone::Math::Vector4D bbMax = Cyclone::Math::Vector4D::sNegInf();

					Cyclone::Math::Matrix44D rotmat = Cyclone::Math::Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( rotation.mPitchYawRoll ) );

					for ( size_t i = 0; i < pathData.mKnots.size(); ++i ) {
						using namespace DirectX;

						const auto &segment = pathData.mKnots[i];

						Cyclone::Math::Vector4D point = rotmat.TransformCoord3Unit( segment.mPoint );
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, point );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, point );

						if ( i != 0 ) {
							Cyclone::Math::Vector4D inVec = rotmat.TransformCoord3Unit( segment.mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( segment.mInVec ) );
							bbMin = Cyclone::Math::Vector4D::sMin( bbMin, inVec );
							bbMax = Cyclone::Math::Vector4D::sMax( bbMax, inVec );
						}

						if ( i + 1 < pathData.mKnots.size() ) {
							Cyclone::Math::Vector4D outVec = rotmat.TransformCoord3Unit( segment.mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( segment.mOutVec ) );
							bbMin = Cyclone::Math::Vector4D::sMin( bbMin, outVec );
							bbMax = Cyclone::Math::Vector4D::sMax( bbMax, outVec );
						}

						Cyclone::Math::Vector4D bitangent = rotmat.TransformCoord3Unit( segment.mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent * 0.5f ) );
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, bitangent );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, bitangent );

						bitangent = rotmat.TransformCoord3Unit( segment.mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent * -0.5f ) );
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, bitangent );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, bitangent );

						bitangent = rotmat.TransformCoord3Unit( segment.mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent * 0.5f + pathData.mExtrusions[i].mNormal * -0.1f ) );
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, bitangent );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, bitangent );

						bitangent = rotmat.TransformCoord3Unit( segment.mPoint + Cyclone::Math::Vector4D::sFromXMVECTOR( pathData.mExtrusions[i].mBitangent * -0.5f + pathData.mExtrusions[i].mNormal * -0.1f ) );
						bbMin = Cyclone::Math::Vector4D::sMin( bbMin, bitangent );
						bbMax = Cyclone::Math::Vector4D::sMax( bbMax, bitangent );
					}
					
					Cyclone::Math::Vector4D half = Cyclone::Math::Vector4D::sReplicate( 0.5 );
					Cyclone::Math::Vector4D bbCenter = ( bbMax + bbMin ) * half;
					Cyclone::Math::Vector4D bbExtent = ( bbMax - bbMin ) * half;
					
					inHandle.patch<BoundingBox>( [bbCenter, bbExtent]( auto &inV ) {
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