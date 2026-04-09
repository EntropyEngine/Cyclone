#include "pch.h"
#include "Cyclone/UI/Tool/PathActiveTool.hpp"

// Math
#include "Cyclone/Math/Matrix.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Components
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
using Cyclone::Core::Component::PathSelection;

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

struct PointRenderData
{
	DirectX::XMFLOAT3 mPoint;
	DirectX::XMFLOAT3 mInVec;
	DirectX::XMFLOAT3 mOutVec;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT3 mBitangent;
	bool mKnotInSelection;
	bool mKnotSelected;
};

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
	auto view = cregistry.view<EntityType, Position, Rotation, PathSelection, PathData, DrawTag>();
	for ( const entt::entity entity : view ) {
		const auto &entityType = view.get<EntityType>( entity );
		const auto &position = view.get<Position>( entity ).mValue;
		const auto &rotation = view.get<Rotation>( entity ).mPitchYawRoll;
		const PathData &pathData = view.get<PathData>( entity );
		const PathSelection &pathSelection = view.get<PathSelection>( entity );

		Matrix44D rotmat = Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( rotation ) );
		Vector4D rebasedEntityPosition = ( position - cameraP );

		bool isPathSelected = selectedEntity == entity;
		bool isPathInSelection = selectedEntities.contains( entity );

		std::vector<PointRenderData> pointRenderData( pathData.mKnots.size() );

		for ( size_t s = 0; s < pathData.mKnots.size(); ++s ) {
			const auto &knot = pathData.mKnots[s];
			const auto &extrusion = pathData.mExtrusions[s];

			DirectX::XMStoreFloat3( &pointRenderData[s].mPoint, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			DirectX::XMStoreFloat3( &pointRenderData[s].mInVec, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( knot.mInVec ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			DirectX::XMStoreFloat3( &pointRenderData[s].mOutVec, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( knot.mOutVec ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			DirectX::XMStoreFloat3( &pointRenderData[s].mNormal, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( extrusion.mNormal ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			DirectX::XMStoreFloat3( &pointRenderData[s].mBitangent, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( extrusion.mBitangent ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			pointRenderData[s].mKnotInSelection = pathSelection.mSelectedKnots.contains( s );
			pointRenderData[s].mKnotSelected = isPathSelected && pathSelection.mCurrentKnot == s;
		}

		for ( size_t s = 0; s < pathData.mKnots.size(); ++s ) {
			inViewportData.ClipToScreen( pointRenderData[s].mPoint );
			inViewportData.ClipToScreen( pointRenderData[s].mInVec );
			inViewportData.ClipToScreen( pointRenderData[s].mOutVec );
			inViewportData.ClipToScreen( pointRenderData[s].mNormal );
			inViewportData.ClipToScreen( pointRenderData[s].mBitangent );
		}

		for ( size_t s = 0; s < pathData.mKnots.size(); ++s ) {
			const PointRenderData &point = pointRenderData[s];

			uint8_t transparency = point.mKnotInSelection ? 255 : 128;
			uint32_t handleColor = IM_COL32( 255, 255, 255, transparency );
			uint32_t shadowColor = IM_COL32( 0, 0, 0, point.mKnotInSelection ? 255 : 0 );

			if ( ortho || point.mPoint.z < 1 ) {
				drawList->AddCircle( { point.mPoint.x, point.mPoint.y }, 7.0f, shadowColor );

				if ( ortho || point.mInVec.z < 1 ) {
					drawList->AddCircle( { point.mInVec.x, point.mInVec.y }, 5.0f, shadowColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mInVec.x, point.mInVec.y }, shadowColor, 3.0f );
				}

				if ( ortho || point.mOutVec.z < 1 ) {
					drawList->AddCircle( { point.mOutVec.x, point.mOutVec.y }, 5.0f, shadowColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mOutVec.x, point.mOutVec.y }, shadowColor, 3.0f );
				}

				if ( ortho || point.mNormal.z < 1 ) {
					drawList->AddRect( { point.mNormal.x - 5.0f, point.mNormal.y - 5.0f }, { point.mNormal.x + 5.0f, point.mNormal.y + 5.0f }, shadowColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mNormal.x, point.mNormal.y }, shadowColor, 3.0f );
				}

				if ( ortho || point.mBitangent.z < 1 ) {
					drawList->AddRect( { point.mBitangent.x - 5.0f, point.mBitangent.y - 5.0f }, { point.mBitangent.x + 5.0f, point.mBitangent.y + 5.0f }, shadowColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mBitangent.x, point.mBitangent.y }, shadowColor, 3.0f );
				}
			}
		}

		for ( size_t s = 0; s < pathData.mKnots.size(); ++s ) {
			const PointRenderData &point = pointRenderData[s];

			uint8_t transparency = point.mKnotInSelection ? 255 : 128;
			uint32_t handleColor = IM_COL32( 255, 255, 255, transparency );
			uint32_t shadowColor = IM_COL32( 0, 0, 0, point.mKnotInSelection ? 255 : 0 );

			auto circleLambda = [drawList, &point]( ImVec2 inPos, float inRadius, uint32_t inHandleColor ) {
				if ( point.mKnotSelected ) {
					drawList->AddCircleFilled( inPos, inRadius, inHandleColor );
				}
				else {
					drawList->AddCircle( inPos, inRadius, inHandleColor );
				}
			};

			auto rectLambda = [drawList, &point]( ImVec2 inPosMin, ImVec2 inPosMax, uint32_t inHandleColor ) {
				if ( point.mKnotSelected ) {
					drawList->AddRectFilled( inPosMin, inPosMax, inHandleColor );
				}
				else {
					drawList->AddRect( inPosMin, inPosMax, inHandleColor );
				}
			};

			if ( ortho || point.mPoint.z < 1 ) {
				circleLambda( { point.mPoint.x, point.mPoint.y }, 6.0f, handleColor );

				if ( ortho || point.mInVec.z < 1 ) {
					circleLambda( { point.mInVec.x, point.mInVec.y }, 4.0f, handleColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mInVec.x, point.mInVec.y }, handleColor, 1.0f );
				}

				if ( ortho || point.mOutVec.z < 1 ) {
					circleLambda( { point.mOutVec.x, point.mOutVec.y }, 4.0f, handleColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mOutVec.x, point.mOutVec.y }, handleColor, 1.0f );
				}

				if ( ortho || point.mNormal.z < 1 ) {
					rectLambda( { point.mNormal.x - 4.0f, point.mNormal.y - 4.0f }, { point.mNormal.x + 4.0f, point.mNormal.y + 4.0f }, handleColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mNormal.x, point.mNormal.y }, handleColor, 1.0f );
				}

				if ( ortho || point.mBitangent.z < 1 ) {
					rectLambda( { point.mBitangent.x - 4.0f, point.mBitangent.y - 4.0f }, { point.mBitangent.x + 4.0f, point.mBitangent.y + 4.0f }, handleColor );
					drawList->AddLine( { point.mPoint.x, point.mPoint.y }, { point.mBitangent.x, point.mBitangent.y }, handleColor, 1.0f );
				}
			}
		}
		
		continue;

		for ( size_t s = 0; s < pathData.mKnots.size(); ++s ) {
			using namespace DirectX;

			const auto &knot = pathData.mKnots[s];
			const auto &extrusion = pathData.mExtrusions[s];

			DirectX::XMFLOAT4 v1, v2;
			ImVec2 p1, p2;

			bool isKnotInSelection = pathSelection.mSelectedKnots.contains( s );

			uint8_t transparency = isKnotInSelection ? 255 : 128;
			uint32_t handleColor = IM_COL32( 255, 255, 255, transparency );
			uint32_t shadowColor = IM_COL32( 0, 0, 0, isKnotInSelection ? 255 : 0 );

			bool isKnotSelected = isPathSelected && pathSelection.mCurrentKnot == s;

			DirectX::XMStoreFloat4( &v1, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );

			DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( knot.mInVec ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
				p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
				p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );

				drawList->AddLine( p1, p2, shadowColor, 3.0f );
				drawList->AddCircle( p1, 7.0f, shadowColor );
				drawList->AddCircle( p2, 5.0f, shadowColor );

				drawList->AddLine( p1, p2, handleColor );
				if ( isKnotSelected ) {
					drawList->AddCircleFilled( p1, 6.0f, handleColor );
					drawList->AddCircleFilled( p2, 4.0f, handleColor );
				}
				else {
					drawList->AddCircle( p1, 6.0f, handleColor );
					drawList->AddCircle( p2, 4.0f, handleColor );
				}
			}

			DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( knot.mOutVec ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
				p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
				p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
				drawList->AddLine( p1, p2, handleColor );
				drawList->AddCircle( p2, 4.0f, handleColor );
			}

			DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( extrusion.mNormal ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
				p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
				p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
				drawList->AddLine( p1, p2, handleColor );
				drawList->AddRect( { p2.x - 4.0f, p2.y - 4.0f }, { p2.x + 4.0f, p2.y + 4.0f }, handleColor );
			}

			DirectX::XMStoreFloat4( &v2, DirectX::XMVector3TransformCoord( ( rotmat.TransformCoord3Unit( knot.mPoint + Vector4D::sFromXMVECTOR( extrusion.mBitangent ) ) + rebasedEntityPosition ).ToXMVECTOR(), ViewProj ) );
			if ( ortho || ( v1.z < 1 && v2.z < 1 ) ) {
				p1 = inViewportData.ClipToScreen( { v1.x, v1.y } );
				p2 = inViewportData.ClipToScreen( { v2.x, v2.y } );
				drawList->AddLine( p1, p2, handleColor );
				drawList->AddRect( { p2.x - 4.0f, p2.y - 4.0f }, { p2.x + 4.0f, p2.y + 4.0f }, handleColor );
			}
		}
	}
}