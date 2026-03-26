#pragma once

#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

namespace Cyclone::Core::Component
{
	struct LocalBounds
	{
		enum class EType { Radius, BoundingBox };

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
					inRegistry.patch<BoundingBox>( inEntity, [newcenter, newextent]( auto &inV ) { inV.mValue.mCenter = Cyclone::Math::Vector4D::sFromXMVECTOR( newcenter ), inV.mValue.mExtent = newextent; } );
					return;
				}
				case EType::BoundingBox: {
					return;
				}
				default:
					assert( false );
					__assume( false );
			}
		}
	};
}