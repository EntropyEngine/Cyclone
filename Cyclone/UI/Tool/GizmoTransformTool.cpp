#include "pch.h"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// DX includes
#include <DebugDraw.h>

using Cyclone::Math::Vector4D;
using Cyclone::Core::Component::Position;

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

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;

	if ( mIsSelected && inViewportData.mIsActive && ImGui::IsMouseDown( ImGuiMouseButton_Left ) && selectedEntity != entt::null ) {
		inPrimitiveBatch->Begin();

		static Vector4D entityOriginalPos = Vector4D::sZero();
		static Vector4D mouseOriginalPos = Vector4D::sZero();

		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			entityOriginalPos = registry.get<Position>( selectedEntity ).mValue;
		}


		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		Vector4D mousePosNear = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( inViewportData.mWorldMouseU, inViewportData.mWorldMouseV, 0.0f, 0.0f ), viewProjInverse ) ) + perspectiveContext.mCenter3D;
		Vector4D mousePosFar = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( inViewportData.mWorldMouseU, inViewportData.mWorldMouseV, 0.99f, 0.0f ), viewProjInverse ) ) + perspectiveContext.mCenter3D;
		Vector4D mouseDelta = mousePosFar - mousePosNear;
		Vector4D mouseDir = mouseDelta.GetNorm3();

		if ( true ) {

			Vector4D axisDir = Vector4D::sZeroSetValueByIndex<2>( 1.0 );
			Vector4D axisA = entityOriginalPos; // Vector4D::sZero();

			Vector4D cameraP = perspectiveContext.mCenter3D;
			Vector4D AP = cameraP - axisA;

			Vector4D cameraProjectedP = axisA + axisDir * Vector4D::sReplicate( AP.Dot3( axisDir ) ); // Todo: can we reduce lane switches?
			Vector4D diffP = ( cameraP - cameraProjectedP );
			Vector4D diffP2 = diffP + diffP;
			Vector4D deltaP = diffP.GetNorm3();

			Vector4D relativeUp = Vector4D::sCross3( axisDir, deltaP );

			Vector4D planeNormal = Vector4D::sCross3( axisDir, relativeUp );
			double planeCoordW = -planeNormal.Dot3( axisA );


			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;


			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			// Inverted plane
			double planeCoordW2 = -planeNormal.Dot3( axisA + diffP2 );
			double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
			Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
			Vector4D planeIntersection2 = intersectionDir2 + mousePosNear;

			double flip = mouseDir.Dot3( intersectionDir.GetNorm3() );
			Vector4D mousePos = planeIntersection;
			if ( flip < 0 ) {
				mousePos = planeIntersection2 - diffP2;
				//mousePos *= ( Vector4D::sReplicate( 1 ) - axisDir ) - axisDir;
			}

			DirectX::XMFLOAT3 mousePos3;
			DirectX::XMStoreFloat3( &mousePos3, ( mousePos - cameraP ).ToXMVECTOR() );

			Vector4D objPos = mousePos * axisDir + axisA * ( Vector4D::sReplicate( 1 ) - axisDir );
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
			DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - perspectiveContext.mCenter3D ).ToXMVECTOR(), viewProj ) );

			size_t size = 1000;
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir.ToXMVECTOR(), size ), DirectX::XMVectorScale( relativeUp.ToXMVECTOR(), size ), ( diffP2 + axisA - cameraP ).ToXMVECTOR(), 2 * size, 2, DirectX::Colors::Gray / 2 );
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir.ToXMVECTOR(), size ), DirectX::XMVectorScale( relativeUp.ToXMVECTOR(), size ), ( axisA - cameraP ).ToXMVECTOR(), 2 * size, 2, DirectX::Colors::Gray / 2 );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

			if ( inViewportData.mIsActive ) ImGui::SetTooltip(
				"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
				inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
				mousePos.GetX(),
				mousePos.GetY(),
				mousePos.GetZ(),
				flip,
				planeVT,
				mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
				reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
			);
		}
		else if ( false ) {
			Vector4D axisDir1 = Vector4D::sZeroSetValueByIndex<2>( 1.0 );
			Vector4D axisDir2 = Vector4D::sZeroSetValueByIndex<0>( 1.0 );
			Vector4D axisMask = axisDir1 + axisDir2;
			Vector4D axisMaskInv = Vector4D::sReplicate( 1.0 ) - axisMask;


			Vector4D cameraP = perspectiveContext.mCenter3D;
			Vector4D axisA = entityOriginalPos; // Vector4D::sZero();

			Vector4D cameraProjectedP = cameraP * axisMask + axisA * axisMaskInv;
			Vector4D diffP = ( cameraP - cameraProjectedP );
			Vector4D diffP2 = diffP + diffP;

			Vector4D planeNormal = Vector4D::sCross3( axisDir1, axisDir2 );
			double planeCoordW = -planeNormal.Dot3( axisA );

			double planeV1 = planeNormal.Dot3( mousePosNear );
			double planeV2 = planeNormal.Dot3( mousePosFar );
			double planeD = planeV1 - planeV2;
			double planeVT = ( planeV1 + planeCoordW ) / planeD;

			Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
			Vector4D planeIntersection = intersectionDir + mousePosNear;

			// Inverted plane
			double planeCoordW2 = -planeNormal.Dot3( axisA + diffP2 );
			double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
			Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
			Vector4D planeIntersection2 = intersectionDir2 + mousePosNear;

			double flip = mouseDir.Dot3( intersectionDir.GetNorm3() );
			Vector4D mousePos = planeIntersection;
			if ( flip < 0 ) {
				mousePos = planeIntersection2 - diffP2;
				//mousePos *= axisMaskInv - axisMask;
			}

			DirectX::XMFLOAT3 mousePos3;
			DirectX::XMStoreFloat3( &mousePos3, ( mousePos - cameraP ).ToXMVECTOR() );

			Vector4D objPos = mousePos * axisMask + axisA * axisMaskInv;
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
			DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - perspectiveContext.mCenter3D ).ToXMVECTOR(), viewProj ) );

			size_t size = 1000;
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( diffP2 + axisA - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( axisA - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

			if ( inViewportData.mIsActive ) ImGui::SetTooltip(
				"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
				inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
				mousePos.GetX(),
				mousePos.GetY(),
				mousePos.GetZ(),
				flip,
				planeVT,
				mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
				reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
			);
		}
		else {
			Vector4D cameraP = perspectiveContext.mCenter3D;
			Vector4D axisA = entityOriginalPos; // Vector4D::sZero();

			Vector4D planeNormal = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR2, DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f ) ) );
			Vector4D axisDir1 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR0, DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f ) ) );
			Vector4D axisDir2 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR1, DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f ) ) );
			double planeCoordW = -planeNormal.Dot3( axisA );

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
			DirectX::XMStoreFloat4( &reprojectedMousePos, DirectX::XMVector3TransformCoord( ( objPos - perspectiveContext.mCenter3D ).ToXMVECTOR(), viewProj ) );

			size_t size = 1000;
			DX::DrawGrid( inPrimitiveBatch, DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( axisA - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
			DX::Draw( inPrimitiveBatch, DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

			if ( inViewportData.mIsActive ) ImGui::SetTooltip(
				"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
				inViewportData.mWorldMouseU, inViewportData.mWorldMouseV,
				mousePos.GetX(),
				mousePos.GetY(),
				mousePos.GetZ(),
				flip,
				planeVT,
				mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
				reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
			);
		}

		inPrimitiveBatch->End();
	}
}