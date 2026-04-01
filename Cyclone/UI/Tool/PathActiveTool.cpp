#include "pch.h"
#include "Cyclone/UI/Tool/PathActiveTool.hpp"

// Math
#include "Cyclone/Math/Matrix.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Compontnts
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// DX includes
#include <DebugDraw.h>

using Cyclone::Math::Vector4D;
using Cyclone::Math::Matrix44D;
using Cyclone::Core::Component::EntityType;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Component::Rotation;
using Cyclone::Core::Component::PathData;
using Cyclone::Core::Component::PathTag;

void Cyclone::UI::Tool::PathActiveTool::OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	if ( mIsSelected ) {
		inViewportData.mDrawList->ChannelsSetCurrent( 3 );
		switch ( inType ) {
			case EViewportType::Perspective: OnDraw<EViewportType::Perspective>( inLevelInterface, inViewportData ); break;
			case EViewportType::TopXZ: OnDraw<EViewportType::TopXZ>( inLevelInterface, inViewportData ); break;
			case EViewportType::FrontXY: OnDraw<EViewportType::FrontXY>( inLevelInterface, inViewportData ); break;
			case EViewportType::SideYZ: OnDraw<EViewportType::SideYZ>( inLevelInterface, inViewportData ); break;
		}
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::PathActiveTool::OnDraw( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	Vector4D cameraP = T == EViewportType::Perspective ? inLevelInterface->GetPerspectiveCtx().mCenter3D : inLevelInterface->GetOrthographicCtx().mCenter2D;

	bool ortho = T != EViewportType::Perspective;

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityManager = inLevelInterface->GetEntityManager();

	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();

	const entt::registry &cregistry = inLevelInterface->GetRegistry();

	DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;
	DirectX::XMMATRIX ViewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );

	ImDrawList *drawList = inViewportData.mDrawList;

	using DrawTag = ViewportTypeTraits<T>::DrawTag;
	auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathData, DrawTag>();
	for ( const entt::entity entity : view ) {
		const auto &entityType = view.get<EntityType>( entity );
		const auto &position = view.get<Position>( entity ).mValue;
		const auto &rotation = view.get<Rotation>( entity ).mPitchYawRoll;
		const PathData &pathData = view.get<PathData>( entity );

		Matrix44D rotmat = Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( rotation ) );
		Vector4D rebasedEntityPosition = ( position - cameraP );

		//uint32_t entityColorU32;
		//if ( entity == selectedEntity ) {
		//	entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
		//}
		//else if ( selectedEntities.contains( entity ) ) {
		//	entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
		//}
		//else {
		//	entityColorU32 = entityManager.GetEntityTypeColor( entityType );
		//}

		for ( const auto &segment : pathData.mPathGuide ) {
			DirectX::XMFLOAT4 v1, v2;
			ImVec2 p1, p2;

			DirectX::XMStoreFloat4( &v1, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( segment.mP0 ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( segment.mP1 ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
				p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
				p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
				drawList->AddLine( p1, p2, IM_COL32( 255, 255, 255, 128 ) );
				drawList->AddCircle( p1, 6.0f, IM_COL32( 255, 255, 255, 128 ) );
				drawList->AddCircle( p2, 4.0f, IM_COL32( 255, 255, 255, 128 ) );
			}

			DirectX::XMStoreFloat4( &v1, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( segment.mP2 ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( segment.mP3 ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
				p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
				p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
				drawList->AddLine( p1, p2, IM_COL32( 255, 255, 255, 128 ) );
				drawList->AddCircle( p1, 4.0f, IM_COL32( 255, 255, 255, 128 ) );
				drawList->AddCircle( p2, 6.0f, IM_COL32( 255, 255, 255, 128 ) );
			}

			Vector4D bitangent = Vector4D( 1.0, 0.0, 0.0 );
			Vector4D half = Vector4D::sReplicate( 0.5 );

			for ( double u = 0.0; u <= 1.0; u += 1.0 / 16 ) {
				Vector4D p = segment.GetPoint( u );
				Vector4D t = segment.GetDerivative( u ).GetNorm3();

				Vector4D normal = Vector4D::sCross3( t, bitangent );
				Vector4D b = Vector4D::sCross3( t, normal );

				Vector4D n2 = Vector4D::sCross3( bitangent, Vector4D::sCross3( b, Vector4D( 0, 1, 0 ) ).GetNorm3() );

				DirectX::XMStoreFloat4( &v1, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( p ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
				DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( p + normal * half ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
				if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
					p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
					p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
					drawList->AddLine( p1, p2, IM_COL32( 255, 0, 0, 255 ) );
				}

				DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( p + bitangent * half ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
				if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
					p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
					drawList->AddLine( p1, p2, IM_COL32( 0, 255, 0, 255 ) );
				}

				DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( p + b * half ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
				if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
					p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
					drawList->AddLine( p1, p2, IM_COL32( 0, 255, 255, 255 ) );
				}

				DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( p + n2 * half ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
				if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
					p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
					drawList->AddLine( p1, p2, IM_COL32( 255, 0, 255, 255 ) );
				}

				//DirectX::XMStoreFloat2( reinterpret_cast<DirectX::XMFLOAT2 *>( &p1 ), DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( p + ( normal + bitangent ) * half ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
				//p1 = inViewportData.ClipToScreen( p1 );
				//drawList->AddLine( p1, p2, IM_COL32( 255, 0, 0, 255 ) );
			}
		}
	}
}