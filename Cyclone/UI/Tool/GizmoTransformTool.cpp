#include "pch.h"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// DX includes
#include <DebugDraw.h>

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

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;


	if ( mIsSelected && selectedEntity != entt::null ) {
		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		Vector4D cameraP = perspectiveContext.mCenter3D;
		Vector4D entityCurrentPosition = registry.get<Position>( selectedEntity ).mValue;
		Vector4D entityCurrentPositionRel = entityCurrentPosition - cameraP;

		// TODO: optimize for SSE lanes
		//
		//

		DirectX::XMMATRIX viewRotation = DirectX::XMMatrixRotationRollPitchYaw( static_cast<float>( perspectiveContext.mCameraPitch ), static_cast<float>( perspectiveContext.mCameraYaw ), 0.0f );
		Vector4D axisDir1 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR0, viewRotation ) );
		Vector4D axisDir2 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR2, viewRotation ) );

		DirectX::XMVECTOR endpointO = entityCurrentPositionRel.ToXMVECTOR();
		DirectX::XMVECTOR endpointO1 = ( entityCurrentPositionRel + axisDir1 ).ToXMVECTOR();

		float O0X = DirectX::XMVectorGetX( DirectX::XMVector3TransformCoord( endpointO, viewProj ) );
		float O1X = DirectX::XMVectorGetX( DirectX::XMVector3TransformCoord( endpointO1, viewProj ) );
		float scale = 128.0f / ( std::max( inViewportData.mViewSize.x, inViewportData.mViewSize.y ) * ( O0X - O1X ) * 0.5f );

		DirectX::XMVECTOR endpointX = ( entityCurrentPositionRel + Vector4D( scale, 0, 0 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointY = ( entityCurrentPositionRel + Vector4D( 0, scale, 0 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointZ = ( entityCurrentPositionRel + Vector4D( 0, 0, scale ) ).ToXMVECTOR();

		Vector4D axisDir2X = Vector4D::sCross3( entityCurrentPositionRel.GetNorm3(), Vector4D( 1, 0, 0 ) ).GetNorm3();
		Vector4D axisDir3X = Vector4D::sCross3( axisDir2X, Vector4D( 1, 0, 0 ) ).GetNorm3();

		DirectX::XMVECTOR endpointXT1 = ( entityCurrentPositionRel + ( Vector4D( 8, 0.0, 0.0 ) + axisDir2X + axisDir3X ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointXT2 = ( entityCurrentPositionRel + ( Vector4D( 8, 0.0, 0.0 ) + axisDir2X - axisDir3X ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointXT3 = ( entityCurrentPositionRel + ( Vector4D( 8, 0.0, 0.0 ) - axisDir2X - axisDir3X ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointXT4 = ( entityCurrentPositionRel + ( Vector4D( 8, 0.0, 0.0 ) - axisDir2X + axisDir3X ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();

		Vector4D axisDir2Y = Vector4D::sCross3( entityCurrentPositionRel.GetNorm3(), Vector4D( 0, 1, 0 ) ).GetNorm3();
		Vector4D axisDir3Y = Vector4D::sCross3( axisDir2Y, Vector4D( 0, 1, 0 ) ).GetNorm3();

		DirectX::XMVECTOR endpointYT1 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 8, 0.0 ) + axisDir2Y + axisDir3Y ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointYT2 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 8, 0.0 ) + axisDir2Y - axisDir3Y ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointYT3 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 8, 0.0 ) - axisDir2Y - axisDir3Y ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointYT4 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 8, 0.0 ) - axisDir2Y + axisDir3Y ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();

		Vector4D axisDir2Z = Vector4D::sCross3( entityCurrentPositionRel.GetNorm3(), Vector4D( 0, 0, 1 ) ).GetNorm3();
		Vector4D axisDir3Z = Vector4D::sCross3( axisDir2Z, Vector4D( 0, 0, 1 ) ).GetNorm3();

		DirectX::XMVECTOR endpointZT1 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 0.0, 8 ) + axisDir2Z + axisDir3Z ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointZT2 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 0.0, 8 ) + axisDir2Z - axisDir3Z ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointZT3 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 0.0, 8 ) - axisDir2Z - axisDir3Z ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();
		DirectX::XMVECTOR endpointZT4 = ( entityCurrentPositionRel + ( Vector4D( 0.0, 0.0, 8 ) - axisDir2Z + axisDir3Z ) * Vector4D::sReplicate( scale * 0.1 ) ).ToXMVECTOR();

		auto transform = [viewProj, inViewportData]( DirectX::XMVECTOR &inPosition ) {
			DirectX::XMFLOAT2 clip;
			DirectX::XMStoreFloat2( &clip, DirectX::XMVector3TransformCoord( inPosition, viewProj ) );

			ImVec2 screen;
			screen.x = clip.x * ( inViewportData.mViewSize.x / 2.0f ) + ( inViewportData.mViewSize.x / 2.0f ) + inViewportData.mViewOrigin.x;
			screen.y = clip.y * ( inViewportData.mViewSize.y / -2.f ) + ( inViewportData.mViewSize.y / 2.0f ) + inViewportData.mViewOrigin.y;

			return screen;
		};

		ImVec2 endpointScreenX = transform( endpointX );
		ImVec2 endpointScreenY = transform( endpointY );
		ImVec2 endpointScreenZ = transform( endpointZ );
		ImVec2 endpointScreenO = transform( endpointO );

		inViewportData.mDrawList->AddLine( endpointScreenO, endpointScreenX, IM_COL32( 255, 0, 0, 255 ), 2.0f );
		inViewportData.mDrawList->AddLine( endpointScreenO, endpointScreenY, IM_COL32( 0, 255, 0, 255 ), 2.0f );
		inViewportData.mDrawList->AddLine( endpointScreenO, endpointScreenZ, IM_COL32( 0, 0, 255, 255 ), 2.0f );

		inViewportData.mDrawList->AddTriangleFilled( endpointScreenX, transform( endpointXT1 ), transform( endpointXT2 ), IM_COL32( 255, 0, 0, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenX, transform( endpointXT2 ), transform( endpointXT3 ), IM_COL32( 255, 0, 0, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenX, transform( endpointXT3 ), transform( endpointXT4 ), IM_COL32( 255, 0, 0, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenX, transform( endpointXT4 ), transform( endpointXT1 ), IM_COL32( 255, 32, 32, 255 ) );

		inViewportData.mDrawList->AddTriangleFilled( endpointScreenY, transform( endpointYT1 ), transform( endpointYT2 ), IM_COL32( 0, 255, 0, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenY, transform( endpointYT2 ), transform( endpointYT3 ), IM_COL32( 0, 255, 0, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenY, transform( endpointYT3 ), transform( endpointYT4 ), IM_COL32( 0, 255, 0, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenY, transform( endpointYT4 ), transform( endpointYT1 ), IM_COL32( 32, 255, 32, 255 ) );

		inViewportData.mDrawList->AddTriangleFilled( endpointScreenZ, transform( endpointZT1 ), transform( endpointZT2 ), IM_COL32( 0, 0, 255, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenZ, transform( endpointZT2 ), transform( endpointZT3 ), IM_COL32( 0, 0, 255, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenZ, transform( endpointZT3 ), transform( endpointZT4 ), IM_COL32( 0, 0, 255, 255 ) );
		inViewportData.mDrawList->AddTriangleFilled( endpointScreenZ, transform( endpointZT4 ), transform( endpointZT1 ), IM_COL32( 32, 32, 255, 255 ) );
	}
}

void Cyclone::UI::Tool::GizmoTransformTool::OnRenderPerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &entityManager = inLevelInterface->GetEntityManager();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;

	gizmoContext.mCurrentAxis = GizmoToolContext::ZAxis;
	//gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::ZAxis;
	//gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::YAxis | GizmoToolContext::ZAxis;

	if ( mIsSelected && inViewportData.mIsActive && ImGui::IsMouseDown( ImGuiMouseButton_Left ) && selectedEntity != entt::null ) {
		inPrimitiveBatch->Begin();


		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			assert( gizmoContext.mActiveEntity == entt::null && "Active entity already set!" );

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

		size_t nGridlines = 1000;

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

			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), static_cast<float>( nGridlines ) ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), static_cast<float>( nGridlines ) ), ( diffP2 + entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * nGridlines, 2 * nGridlines, DirectX::Colors::Gray / 2 );
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

			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), static_cast<float>( nGridlines ) ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), static_cast<float>( nGridlines ) ), ( diffP2 + entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * nGridlines, 2 * nGridlines, DirectX::Colors::Gray / 2 );
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


		DirectX::XMFLOAT3 mousePos3;
		DirectX::XMStoreFloat3( &mousePos3, ( mousePos - cameraP ).ToXMVECTOR() );

		Vector4D objPos = mousePos * axisMask + entityOriginalPos * axisMaskInv;
		DirectX::XMFLOAT3 objPos3;
		DirectX::XMStoreFloat3( &objPos3, ( objPos - cameraP ).ToXMVECTOR() );

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

		DirectX::XMFLOAT3 objPosNew3;
		DirectX::XMStoreFloat3( &objPosNew3, ( objPosNew - cameraP ).ToXMVECTOR() );

		for ( const entt::entity entity : selectedEntities ) {
			registry.patch<Position>( entity, [perFrameDelta]( Position &inPosition ) { inPosition.mValue += perFrameDelta; } );
		}

		DirectX::XMFLOAT4 reprojectedMousePos;
		DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - cameraP ).ToXMVECTOR(), viewProj ) );

		DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), static_cast<float>( nGridlines ) ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), static_cast<float>( nGridlines ) ), ( entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * nGridlines, 2 * nGridlines, DirectX::Colors::Gray / 2 );
		DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1f, .1f, .1f ) ) );
		DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1f, .1f, .1f ) ) );
		DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1f, .1f, .1f ) ) );

		if ( inViewportData.mIsActive ) ImGui::SetTooltip(
			"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
			inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
			mousePos.GetX(),
			mousePos.GetY(),
			mousePos.GetZ(),
			mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
			reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
		);

		inPrimitiveBatch->End();
	}
	else if ( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) && gizmoContext.mActiveEntity != entt::null ) {
		for ( const entt::entity entity : selectedEntities ) {
			entityManager.UpdateEntity( entity, registry );
		}
		entityManager.EndAction( registry );
		gizmoContext.Deactivate();
	}
}