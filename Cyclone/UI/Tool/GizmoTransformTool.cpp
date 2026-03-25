#include "pch.h"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// DX includes
#include <DebugDraw.h>

// ImGuizmo
#include <ImGuizmo/ImGuizmo.h>

// STL
#include <bit>
#include <format>

using Cyclone::Math::Vector4D;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Tool::GizmoToolContext;

void Cyclone::UI::Tool::GizmoTransformTool::OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	switch ( inType ) {
		case EViewportType::Perspective: OnUpdatePerspective( inLevelInterface, inViewportData ); break;
	}
}

void Cyclone::UI::Tool::GizmoTransformTool::OnRender( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	switch ( inType ) {
		case EViewportType::Perspective: OnRenderPerspective( inLevelInterface, inViewportData, inPrimitiveBatch ); break;
	}
}

void Cyclone::UI::Tool::GizmoTransformTool::OnUpdatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &entityManager = inLevelInterface->GetEntityManager();
	auto &transformContext = inLevelInterface->GetSelectionTransformCtx();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;


	if ( mIsSelected && selectedEntity != entt::null ) {
		Vector4D cameraP = perspectiveContext.mCenter3D;
		Vector4D entityCurrentPosition = registry.get<Position>( selectedEntity ).mValue;
		Vector4D entityCurrentPositionRel = entityCurrentPosition - cameraP;

		ImGuizmo::SetGizmoSizeClipSpace( 192.0f / std::max( inViewportData.mViewSize.x, inViewportData.mViewSize.y ) );

		ImGuizmo::PushID( static_cast<int>( selectedEntity ) );
		ImGuizmo::SetRect( inViewportData.mViewOrigin.x, inViewportData.mViewOrigin.y, inViewportData.mViewSize.x, inViewportData.mViewSize.y );
		ImGuizmo::SetDrawlist( inViewportData.mDrawList );
		ImGuizmo::SetOrthographic( false );
		ImGuizmo::Enable( true );
		ImGuizmo::AllowAxisFlip( false );

		DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixTranslationFromVector( entityCurrentPositionRel.ToXMVECTOR() );

		ImGuizmo::Manipulate( reinterpret_cast<const float *>( &viewMatrix ), reinterpret_cast<const float *>( &projMatrix ), ImGuizmo::TRANSLATE, ImGuizmo::WORLD, reinterpret_cast<float *>( &modelMatrix ) );

		ImGuizmo::PopID();
	}

	ImGuizmo::PushID( static_cast<int>( selectedEntity ) );
	if ( mIsSelected && inViewportData.mIsActive && ImGuizmo::IsUsing() && selectedEntity != entt::null ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			assert( gizmoContext.mActiveEntity == entt::null && "Active entity already set!" );

			gizmoContext.mCurrentAxis = 0;
			ImGuizmo::MOVETYPE moveType = ImGuizmo::GetMoveType();

			switch ( moveType ) {
				case ImGuizmo::MT_MOVE_X: gizmoContext.mCurrentAxis = GizmoToolContext::XAxis; break;
				case ImGuizmo::MT_MOVE_Y: gizmoContext.mCurrentAxis = GizmoToolContext::YAxis; break;
				case ImGuizmo::MT_MOVE_Z: gizmoContext.mCurrentAxis = GizmoToolContext::ZAxis; break;
				case ImGuizmo::MT_MOVE_YZ: gizmoContext.mCurrentAxis = GizmoToolContext::YAxis | GizmoToolContext::ZAxis; break;
				case ImGuizmo::MT_MOVE_ZX: gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::ZAxis; break;
				case ImGuizmo::MT_MOVE_XY: gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::YAxis; break;
				case ImGuizmo::MT_MOVE_SCREEN: gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::YAxis | GizmoToolContext::ZAxis; break;
				default:
					assert( false );
					__assume( false );
			}

			entityManager.BeginAction();

			gizmoContext.mInitialEntityPosition = registry.get<Position>( selectedEntity ).mValue;
			gizmoContext.mActiveEntity = selectedEntity;
		}

		assert( gizmoContext.mActiveEntity == selectedEntity && "Active entity missmatch!" );

		Vector4D entityOriginalPos = gizmoContext.mInitialEntityPosition;


		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		Vector4D cameraP = perspectiveContext.mCenter3D;

		Vector4D mousePosNear = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( static_cast<float>( inViewportData.mWorldMouseU ), static_cast<float>( inViewportData.mWorldMouseV ), 0.0f, 0.0f ), viewProjInverse ) ) + cameraP;
		Vector4D mousePosFar = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( static_cast<float>( inViewportData.mWorldMouseU ), static_cast<float>( inViewportData.mWorldMouseV ), 0.99f, 0.0f ), viewProjInverse ) ) + cameraP;
		Vector4D mouseDelta = mousePosFar - mousePosNear;
		Vector4D mouseDir = mouseDelta.GetNorm3();

		Vector4D axisDir1{ nullptr };
		Vector4D axisDir2{ nullptr };
		Vector4D axisMask{ nullptr };
		Vector4D axisMaskInv{ nullptr };
		Vector4D mousePos{ nullptr };

		int axisBitcount = std::popcount( gizmoContext.mCurrentAxis );

		// Single axis transform
		if ( axisBitcount == 1 ) {

			axisDir1 = Vector4D(
				( gizmoContext.mCurrentAxis & GizmoToolContext::XAxis ) ? 1.0 : 0.0,
				( gizmoContext.mCurrentAxis & GizmoToolContext::YAxis ) ? 1.0 : 0.0,
				( gizmoContext.mCurrentAxis & GizmoToolContext::ZAxis ) ? 1.0 : 0.0
			);
			axisMask = axisDir1;
			axisMaskInv = Vector4D::sReplicate( 1.0 ) - axisMask;

			Vector4D AP = cameraP - entityOriginalPos;

			Vector4D cameraProjectedP = entityOriginalPos + axisDir1 * Vector4D::sReplicate( AP.Dot3( axisDir1 ) ); // Todo: can we reduce lane switches?
			Vector4D diffP = ( cameraP - cameraProjectedP );
			Vector4D diffP2 = diffP + diffP;
			Vector4D deltaP = diffP.GetNorm3();

			axisDir2 = Vector4D::sCross3( axisDir1, deltaP );

			Vector4D planeNormal = Vector4D::sCross3( axisDir1, axisDir2 );
			double planeCoordW = -planeNormal.Dot3( entityOriginalPos );

			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;

			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			double flip = planeVT; // mouseDir.Dot3( intersectionDir.GetNorm3() );
			mousePos = planeIntersection;

			// Inverted plane
			if ( flip < 0 ) {
				double planeCoordW2 = -planeNormal.Dot3( entityOriginalPos + diffP2 );
				double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
				Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
				Vector4D planeIntersection2 = intersectionDir2 + mousePosNear;

				mousePos = planeIntersection2 - diffP2;
			}

			if ( !std::isfinite( flip ) ) {
				mousePos = gizmoContext.mInitialMousePosition;
			}
		}
		// Two axis transform
		else if ( axisBitcount == 2 ) {
			bool first = true;
			for ( int i = 0; i < 3; ++i ) {
				if ( gizmoContext.mCurrentAxis & ( 1 << i ) ) {
					( first ? axisDir1 : axisDir2 ) = Vector4D::sZeroSetValueByIndex( i, 1.0 );
					first = false;
				}
			}

			axisMask = axisDir1 + axisDir2;
			axisMaskInv = Vector4D::sReplicate( 1.0 ) - axisMask;

			Vector4D cameraProjectedP = cameraP * axisMask + entityOriginalPos * axisMaskInv;
			Vector4D diffP = ( cameraP - cameraProjectedP );
			Vector4D diffP2 = diffP + diffP;

			Vector4D planeNormal = Vector4D::sCross3( axisDir1, axisDir2 );
			double planeCoordW = -planeNormal.Dot3( entityOriginalPos );

			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;

			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			double flip = planeVT; // mouseDir.Dot3( intersectionDir.GetNorm3() );
			mousePos = planeIntersection;

			// Inverted plane
			if ( flip < 0 ) {
				double planeCoordW2 = -planeNormal.Dot3( entityOriginalPos + diffP2 );
				double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
				Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
				Vector4D planeIntersection2 = intersectionDir2 + mousePosNear;

				mousePos = planeIntersection2 - diffP2;
			}

			if ( !std::isfinite( flip ) ) {
				mousePos = gizmoContext.mInitialMousePosition;
			}
		}
		// Camera aligned transform
		else if ( axisBitcount == 3 ) {
			DirectX::XMMATRIX viewRotation = DirectX::XMMatrixRotationRollPitchYaw( static_cast<float>( perspectiveContext.mCameraPitch ), static_cast<float>( perspectiveContext.mCameraYaw ), 0.0f );
			axisDir1 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR0, viewRotation ) );
			axisDir2 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR1, viewRotation ) );

			Vector4D planeNormal = Vector4D::sCross3( axisDir1, axisDir2 );
			double planeCoordW = -planeNormal.Dot3( entityOriginalPos );

			axisMask = Vector4D::sReplicate( 1 );
			axisMaskInv = Vector4D::sZero();

			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;

			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			double flip = mouseDir.Dot3( intersectionDir.GetNorm3() );
			mousePos = planeIntersection;
			if ( flip < 0 ) {
				assert( false );
			}
		}
		else {
			assert( false );
			__assume( false );
		}

		Vector4D objPos = mousePos * axisMask + entityOriginalPos * axisMaskInv;

		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			gizmoContext.mInitialMousePosition = objPos;
		}
		Vector4D mouseOriginalPos = gizmoContext.mInitialMousePosition;

		Vector4D objPosDelta = objPos - mouseOriginalPos;

		Vector4D gridSize = Vector4D::sReplicate( gridContext.mGridSize );
		Vector4D gridSizeInv = Vector4D::sReplicate( 1.0 / gridContext.mGridSize );

		if ( gridContext.mSnapType == Cyclone::Core::Editor::GridContext::ESnapType::ByGrid ) {
			objPosDelta = objPosDelta * axisMaskInv + Vector4D::sRound( objPosDelta * gridSizeInv ) * gridSize * axisMask;
		}

		Vector4D objPosNew = objPosDelta + entityOriginalPos;
		objPosNew = Vector4D::sClamp( objPosNew, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

		if ( gridContext.mSnapType == Cyclone::Core::Editor::GridContext::ESnapType::ToGrid ) {
			objPosNew = objPosNew * axisMaskInv + Vector4D::sRound( objPosNew * gridSizeInv ) * gridSize * axisMask;
		}

		Vector4D perFrameDelta = objPosNew - registry.get<Position>( selectedEntity ).mValue;

		for ( const entt::entity entity : selectedEntities ) {
			registry.patch<Position>( entity, [perFrameDelta]( Position &inPosition ) { inPosition.mValue += perFrameDelta; } );
		}
		transformContext.UpdateOnDrag( perFrameDelta );
	}
	else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && gizmoContext.mActiveEntity != entt::null ) {
		for ( const entt::entity entity : selectedEntities ) {
			entityManager.UpdateEntity( entity, registry );
		}
		entityManager.EndAction( registry );
		gizmoContext.Deactivate();
	}
	ImGuizmo::PopID();
}

void Cyclone::UI::Tool::GizmoTransformTool::OnRenderPerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	
}