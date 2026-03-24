#include "pch.h"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// DX includes
#include <DebugDraw.h>

// STL
#include <bit>

using Cyclone::Math::Vector4D;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Tool::GizmoToolContext;

void Cyclone::UI::Tool::GizmoTransformTool::OnRender( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	switch ( inType ) {
		case EViewportType::Perspective: OnRenderPerspective( inLevelInterface, inViewportData, inPrimitiveBatch );
	}
}


void Cyclone::UI::Tool::GizmoTransformTool::OnRenderPerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;

	gizmoContext.mCurrentAxis = GizmoToolContext::ZAxis;
	//gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::ZAxis;
	//gizmoContext.mCurrentAxis = GizmoToolContext::XAxis | GizmoToolContext::YAxis | GizmoToolContext::ZAxis;

	if ( mIsSelected && inViewportData.mIsActive && ImGui::IsMouseDown( ImGuiMouseButton_Left ) && selectedEntity != entt::null ) {
		inPrimitiveBatch->Begin();

		static Vector4D entityOriginalPos = Vector4D::sZero();
		static Vector4D mouseOriginalPos = Vector4D::sZero();

		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			entityOriginalPos = registry.get<Position>( selectedEntity ).mValue;
		}


		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		Vector4D cameraP = perspectiveContext.mCenter3D;

		Vector4D mousePosNear = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( inViewportData.mWorldMouseU, inViewportData.mWorldMouseV, 0.0f, 0.0f ), viewProjInverse ) ) + cameraP;
		Vector4D mousePosFar = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( inViewportData.mWorldMouseU, inViewportData.mWorldMouseV, 0.99f, 0.0f ), viewProjInverse ) ) + cameraP;
		Vector4D mouseDelta = mousePosFar - mousePosNear;
		Vector4D mouseDir = mouseDelta.GetNorm3();

		int axisBitcount = std::popcount( gizmoContext.mCurrentAxis );

		// Single axis transform
		if ( axisBitcount == 1 ) {

			Vector4D axisDir = Vector4D(
				( gizmoContext.mCurrentAxis & GizmoToolContext::XAxis ) ? 1.0 : 0.0,
				( gizmoContext.mCurrentAxis & GizmoToolContext::YAxis ) ? 1.0 : 0.0,
				( gizmoContext.mCurrentAxis & GizmoToolContext::ZAxis ) ? 1.0 : 0.0
			);
			Vector4D axisMaskInv = Vector4D::sReplicate( 1.0 ) - axisDir;

			Vector4D AP = cameraP - entityOriginalPos;

			Vector4D cameraProjectedP = entityOriginalPos + axisDir * Vector4D::sReplicate( AP.Dot3( axisDir ) ); // Todo: can we reduce lane switches?
			Vector4D diffP = ( cameraP - cameraProjectedP );
			Vector4D diffP2 = diffP + diffP;
			Vector4D deltaP = diffP.GetNorm3();

			Vector4D relativeUp = Vector4D::sCross3( axisDir, deltaP );

			Vector4D planeNormal = Vector4D::sCross3( axisDir, relativeUp );
			double planeCoordW = -planeNormal.Dot3( entityOriginalPos );

			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;

			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			double flip = planeVT; // mouseDir.Dot3( intersectionDir.GetNorm3() );
			Vector4D mousePos = planeIntersection;

			// Inverted plane
			if ( flip < 0 ) {
				double planeCoordW2 = -planeNormal.Dot3( entityOriginalPos + diffP2 );
				double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
				Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
				Vector4D planeIntersection2 = intersectionDir2 + mousePosNear;

				mousePos = planeIntersection2 - diffP2;
			}

			DirectX::XMFLOAT3 mousePos3;
			DirectX::XMStoreFloat3( &mousePos3, ( mousePos - cameraP ).ToXMVECTOR() );

			Vector4D objPos = mousePos * axisDir + entityOriginalPos * axisMaskInv;
			DirectX::XMFLOAT3 objPos3;
			DirectX::XMStoreFloat3( &objPos3, ( objPos - cameraP ).ToXMVECTOR() );

			if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				mouseOriginalPos = objPos;
			}

			Vector4D objPosDelta = objPos - mouseOriginalPos;


			Vector4D objPosNew = objPosDelta + entityOriginalPos;
			objPosNew = Vector4D::sClamp( objPosNew, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

			DirectX::XMFLOAT3 objPosNew3;
			DirectX::XMStoreFloat3( &objPosNew3, ( objPosNew - cameraP ).ToXMVECTOR() );

			registry.get<Position>( selectedEntity ).mValue = objPosNew;

			DirectX::XMFLOAT4 reprojectedMousePos;
			DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - cameraP ).ToXMVECTOR(), viewProj ) );

			size_t size = 1000;
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir.ToXMVECTOR(), size ), DirectX::XMVectorScale( relativeUp.ToXMVECTOR(), size ), ( diffP2 + entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * size, 2, DirectX::Colors::Gray / 2 );
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir.ToXMVECTOR(), size ), DirectX::XMVectorScale( relativeUp.ToXMVECTOR(), size ), ( entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * size, 2, DirectX::Colors::Gray / 2 );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

			if ( inViewportData.mIsActive ) ImGui::SetTooltip(
				"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
				inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
				mousePos.GetX(),
				mousePos.GetY(),
				mousePos.GetZ(),
				mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
				reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
			);
		}
		// Two axis transform
		else if ( axisBitcount == 2 ) {
			Vector4D axisDir1( nullptr );
			Vector4D axisDir2( nullptr );

			bool first = true;
			for ( int i = 0; i < 3; ++i ) {
				if ( gizmoContext.mCurrentAxis & ( 1 << i ) ) {
					( first ? axisDir1 : axisDir2 ) = Vector4D::sZeroSetValueByIndex( i, 1.0 );
					first = false;
				}
			}

			Vector4D axisMask = axisDir1 + axisDir2;
			Vector4D axisMaskInv = Vector4D::sReplicate( 1.0 ) - axisMask;

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
			Vector4D mousePos = planeIntersection;

			// Inverted plane
			if ( flip < 0 ) {
				double planeCoordW2 = -planeNormal.Dot3( entityOriginalPos + diffP2 );
				double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
				Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
				Vector4D planeIntersection2 = intersectionDir2 + mousePosNear;

				mousePos = planeIntersection2 - diffP2;
			}

			DirectX::XMFLOAT3 mousePos3;
			DirectX::XMStoreFloat3( &mousePos3, ( mousePos - cameraP ).ToXMVECTOR() );

			Vector4D objPos = mousePos * axisMask + entityOriginalPos * axisMaskInv;
			DirectX::XMFLOAT3 objPos3;
			DirectX::XMStoreFloat3( &objPos3, ( objPos - cameraP ).ToXMVECTOR() );

			if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				mouseOriginalPos = objPos;
			}

			Vector4D objPosDelta = objPos - mouseOriginalPos;


			Vector4D objPosNew = objPosDelta + entityOriginalPos;
			objPosNew = Vector4D::sClamp( objPosNew, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

			DirectX::XMFLOAT3 objPosNew3;
			DirectX::XMStoreFloat3( &objPosNew3, ( objPosNew - cameraP ).ToXMVECTOR() );

			registry.get<Position>( selectedEntity ).mValue = objPosNew;

			DirectX::XMFLOAT4 reprojectedMousePos;
			DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - cameraP ).ToXMVECTOR(), viewProj ) );

			size_t size = 1000;
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( diffP2 + entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

			if ( inViewportData.mIsActive ) ImGui::SetTooltip(
				"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
				inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
				mousePos.GetX(),
				mousePos.GetY(),
				mousePos.GetZ(),
				mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
				reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
			);
		}
		// Camera aligned transform
		else if ( axisBitcount == 3 ) {
			Vector4D planeNormal = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR2, DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f ) ) );
			Vector4D axisDir1 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR0, DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f ) ) );
			Vector4D axisDir2 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR1, DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f ) ) );
			double planeCoordW = -planeNormal.Dot3( entityOriginalPos );

			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;

			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			double flip = mouseDir.Dot3( intersectionDir.GetNorm3() );
			Vector4D mousePos = planeIntersection;
			if ( flip < 0 ) {
				assert( false );
			}

			DirectX::XMFLOAT3 mousePos3;
			DirectX::XMStoreFloat3( &mousePos3, ( mousePos - cameraP ).ToXMVECTOR() );

			Vector4D objPos = mousePos;
			DirectX::XMFLOAT3 objPos3;
			DirectX::XMStoreFloat3( &objPos3, ( objPos - cameraP ).ToXMVECTOR() );

			if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				mouseOriginalPos = objPos;
			}

			Vector4D objPosDelta = objPos - mouseOriginalPos;


			Vector4D objPosNew = objPosDelta + entityOriginalPos;
			objPosNew = Vector4D::sClamp( objPosNew, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

			DirectX::XMFLOAT3 objPosNew3;
			DirectX::XMStoreFloat3( &objPosNew3, ( objPosNew - cameraP ).ToXMVECTOR() );

			registry.get<Position>( selectedEntity ).mValue = objPosNew;

			DirectX::XMFLOAT4 reprojectedMousePos;
			DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - cameraP ).ToXMVECTOR(), viewProj ) );

			size_t size = 1000;
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( entityOriginalPos - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

			if ( inViewportData.mIsActive ) ImGui::SetTooltip(
				"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
				inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
				mousePos.GetX(),
				mousePos.GetY(),
				mousePos.GetZ(),
				mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
				reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
			);
		}

		inPrimitiveBatch->End();
	}
}