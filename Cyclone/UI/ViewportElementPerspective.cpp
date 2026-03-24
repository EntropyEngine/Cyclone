#include "pch.h"
#include "Cyclone/UI/ViewportElementPerspective.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"

// ImGui Includes
#include <imgui_internal.h>

// DX includes
#include <DebugDraw.h>

using Cyclone::Math::Vector4D;

using Cyclone::Core::Component::EntityType;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Component::BoundingBox;

namespace
{
	DirectX::XMMATRIX XM_CALLCONV GetViewMatrix( float inPitch, float inYaw )
	{
		return DirectX::XMMatrixLookToRH( DirectX::g_XMZero, DirectX::XMVector3Transform( DirectX::g_XMIdentityR2, DirectX::XMMatrixRotationRollPitchYaw( inPitch, inYaw, 0.0f ) ), DirectX::g_XMIdentityR1 );
	}

	DirectX::XMMATRIX XM_CALLCONV GetProjMatrix( size_t inWidth, size_t inHeight, float inFovH, double inWorldLimit )
	{
		float aspectRatio = static_cast<float>( inWidth ) / static_cast<float>( inHeight );
		float fovY = 2.0f * std::atan( std::tan( inFovH / 2 ) / aspectRatio );
		return DirectX::XMMatrixPerspectiveFovRH( fovY, aspectRatio, 0.1f, static_cast<float>( 2 * inWorldLimit ) );
	}
}

void Cyclone::UI::ViewportElementPerspective::UpdateNavigation( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	ImVec2 &viewSize = mViewportData.mViewSize;

	auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::Image( GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	mViewportData.mCanvasID = ImGui::GetItemID();

	const bool isCanvasHovered = ImGui::IsItemHovered();
	const bool isActive = ImGui::IsItemActive();

	mViewportData.mIsActive = ImGui::GetCurrentContext()->ActiveIdWindow == ImGui::GetCurrentWindow() || ( ImGui::GetCurrentContext()->ActiveIdWindow == nullptr && isCanvasHovered );

	if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
		perspectiveContext.mCameraPitch += io.MouseDelta.y * kMouseSensitivity;
		perspectiveContext.mCameraYaw -= io.MouseDelta.x * kMouseSensitivity;

		constexpr float pitchLimit = DirectX::XM_PIDIV2 - 0.01f;
		perspectiveContext.mCameraPitch = std::clamp( perspectiveContext.mCameraPitch, -pitchLimit, pitchLimit );
		perspectiveContext.mCameraYaw = perspectiveContext.mCameraYaw - DirectX::XM_2PI * std::floor( perspectiveContext.mCameraYaw / DirectX::XM_2PI );

		float forward = 0.0f;
		float left = 0.0f;

		forward += ImGui::IsKeyDown( ImGuiKey_W );
		forward -= ImGui::IsKeyDown( ImGuiKey_S );
		left += ImGui::IsKeyDown( ImGuiKey_A );
		left -= ImGui::IsKeyDown( ImGuiKey_D );

		forward *= kKeyboardSensitivity * inDeltaTime;
		left *= kKeyboardSensitivity * inDeltaTime;

		if ( forward || left ) {
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f );
			perspectiveContext.mCenter3D += Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::XMVectorSet( left, 0, forward, 0 ), rotationMatrix ) );
		}
	}

	if ( mViewportData.mIsActive ) {
		float scroll = io.MouseWheel;
		scroll *= kCameraDollySensitivity;
		if ( scroll ) {
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw, 0.0f );
			perspectiveContext.mCenter3D += Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::XMVectorSet( 0, 0, scroll, 0 ), rotationMatrix ) );
		}
	}

	ImVec2 viewportAbsMousePos( io.MousePos.x - mViewportData.mViewOrigin.x, io.MousePos.y - mViewportData.mViewOrigin.y );
	ImVec2 viewportRelMousePos( viewportAbsMousePos.x - viewSize.x / 2.0f, viewportAbsMousePos.y - viewSize.y / 2.0f );
	mViewportData.mWorldMouseU = viewportRelMousePos.x / ( viewSize.x / 2.0f );
	mViewportData.mWorldMouseV = -viewportRelMousePos.y / ( viewSize.y / 2.0f );

	const auto &gridContext = inLevelInterface->GetGridCtx();

	// Update matrices
	mViewportData.mViewMatrix = GetViewMatrix( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw );
	mViewportData.mProjMatrix = GetProjMatrix( mWidth, mHeight, kHorizontalFOV, gridContext.mWorldLimit );
}

void Cyclone::UI::ViewportElementPerspective::UpdateTools( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	for ( auto &tool : inTools ) {
		tool->OnUpdate( EViewportType::Perspective, inLevelInterface, mViewportData );
	}
}

void Cyclone::UI::ViewportElementPerspective::DrawGizmos( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	for ( auto &tool : inTools ) {
		tool->OnDraw( EViewportType::Perspective, inLevelInterface, mViewportData );
	}
}

void Cyclone::UI::ViewportElementPerspective::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();

	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mCommonStates->Wireframe() : mWireframeRSS.Get() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	DirectX::XMMATRIX viewMatrix = mViewportData.mViewMatrix;
	DirectX::XMMATRIX projMatrix = mViewportData.mProjMatrix;

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), viewMatrix, projMatrix );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		mWireframeGridBatch->DrawLine(
			{ ( -perspectiveContext.mCenter3D ).ToXMVECTOR(), DirectX::Colors::DarkRed },
			{ ( -perspectiveContext.mCenter3D + Vector4D::sZeroSetValueByIndex<0>( 1 ) ).ToXMVECTOR(), DirectX::Colors::DarkRed }
		);

		mWireframeGridBatch->DrawLine(
			{ ( -perspectiveContext.mCenter3D ).ToXMVECTOR(), DirectX::Colors::Green },
			{ ( -perspectiveContext.mCenter3D + Vector4D::sZeroSetValueByIndex<1>( 1 ) ).ToXMVECTOR(), DirectX::Colors::Green }
		);

		mWireframeGridBatch->DrawLine(
			{ ( -perspectiveContext.mCenter3D ).ToXMVECTOR(), DirectX::Colors::DarkBlue },
			{ ( -perspectiveContext.mCenter3D + Vector4D::sZeroSetValueByIndex<2>( 1 ) ).ToXMVECTOR(), DirectX::Colors::DarkBlue }
		);
	}
	mWireframeGridBatch->End();

	// Switch to depth buffer
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthDefault(), 0 );

	// Call all tool renders with depth enables
	for ( auto &tool : inTools ) {
		tool->OnRender( EViewportType::Perspective, inLevelInterface, mViewportData, mWireframeGridBatch.get() );
	}

	// Gizmo tests
	if constexpr ( false ) {
		mWireframeGridBatch->Begin();

		DirectX::XMVECTOR axis = DirectX::XMVectorSetZ( DirectX::XMVectorZero(), 1 );

		DirectX::XMVECTOR P = DirectX::XMVectorZero();
		DirectX::XMVECTOR A = ( -perspectiveContext.mCenter3D ).ToXMVECTOR();
		DirectX::XMVECTOR B = DirectX::XMVectorAdd( A, axis );

		DirectX::XMVECTOR AP = DirectX::XMVectorSubtract( P, A );
		DirectX::XMVECTOR AB = DirectX::XMVectorSubtract( B, A );

		DirectX::XMVECTOR projPoint = DirectX::XMVectorAdd( A, DirectX::XMVectorMultiply( DirectX::XMVectorDivide( DirectX::XMVector3Dot( AP, AB ), DirectX::XMVector3Dot( AB, AB ) ), AB ) );
		DirectX::XMFLOAT3 projPoint3;
		DirectX::XMStoreFloat3( &projPoint3, projPoint );

		DirectX::XMVECTOR deltaP = DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( P, projPoint ) );

		DirectX::XMVECTOR relativeUp = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( axis, deltaP ) );

		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		DirectX::XMVECTOR mousePosNear = DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( mViewportData.mWorldMouseU, mViewportData.mWorldMouseV, 0.0f, 0.0f ), viewProjInverse );
		DirectX::XMVECTOR mousePosFar = DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( mViewportData.mWorldMouseU, mViewportData.mWorldMouseV, 0.99f, 0.0f ), viewProjInverse );

		DirectX::XMVECTOR mouseDir = DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( mousePosFar, mousePosNear ) );

		DirectX::XMVECTOR planeEquation = DirectX::XMPlaneFromPoints( A, DirectX::XMVectorAdd( A, axis ), DirectX::XMVectorAdd( A, relativeUp ) );

		DirectX::XMVECTOR planeIntersection = DirectX::XMPlaneIntersectLine( planeEquation, mousePosNear, mousePosFar );
		DirectX::XMFLOAT3 mousePos3;
		DirectX::XMStoreFloat3( &mousePos3, planeIntersection );

		DirectX::XMVECTOR intersectionDir = DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( planeIntersection, mousePosNear ) );

		float flip = DirectX::XMVectorGetX( DirectX::XMVector3Dot( mouseDir, intersectionDir ) );

		mousePos3.x *= flip;
		mousePos3.y *= flip;
		mousePos3.z *= flip;

		size_t size = 1000;
		DX::DrawGrid( mWireframeGridBatch.get(), DirectX::XMVectorScale( axis, size ), DirectX::XMVectorScale( relativeUp, size ), A, 2 * size, 2, DirectX::Colors::Gray );
		DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( projPoint3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
		DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
		DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( DirectX::XMFLOAT3( -perspectiveContext.mCenter3D.GetX(), -perspectiveContext.mCenter3D.GetY(), mousePos3.z ), DirectX::XMFLOAT3( .1, .1, .1 ) ) );
		mWireframeGridBatch->End();

		if ( mViewportData.mIsActive ) ImGui::SetTooltip(
			"(%.2f, %.2f)\n(%.2f, %.2f, %.2f)\nDot=(%.2f)",
			mViewportData.mWorldMouseU, mViewportData.mWorldMouseV,
			mousePos3.x + flip * perspectiveContext.mCenter3D.GetX(),
			mousePos3.y + flip * perspectiveContext.mCenter3D.GetY(),
			mousePos3.z + flip * perspectiveContext.mCenter3D.GetZ(),
			flip
		);
	}
	else {
		const auto &selectionContext = inLevelInterface->GetSelectionCtx();
		entt::entity selectedEntity = selectionContext.GetSelectedEntity();
		entt::registry &registry = inLevelInterface->GetRegistry();

		if ( mViewportData.mIsActive && ImGui::IsMouseDown( ImGuiMouseButton_Left ) && selectedEntity != entt::null ) {
			mWireframeGridBatch->Begin();

			static Vector4D entityOriginalPos = Vector4D::sZero();
			static Vector4D mouseOriginalPos = Vector4D::sZero();

			if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				entityOriginalPos = registry.get<Position>( selectedEntity ).mValue;
			}


			DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
			DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

			Vector4D mousePosNear = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( mViewportData.mWorldMouseU, mViewportData.mWorldMouseV, 0.0f, 0.0f ), viewProjInverse ) ) + perspectiveContext.mCenter3D;
			Vector4D mousePosFar = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( mViewportData.mWorldMouseU, mViewportData.mWorldMouseV, 0.99f, 0.0f ), viewProjInverse ) ) + perspectiveContext.mCenter3D;
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
				DX::DrawGrid( mWireframeGridBatch.get(), DirectX::XMVectorScale( axisDir.ToXMVECTOR(), size ), DirectX::XMVectorScale( relativeUp.ToXMVECTOR(), size ), ( diffP2 + axisA - cameraP ).ToXMVECTOR(), 2 * size, 2, DirectX::Colors::Gray / 2 );
				DX::DrawGrid( mWireframeGridBatch.get(), DirectX::XMVectorScale( axisDir.ToXMVECTOR(), size ), DirectX::XMVectorScale( relativeUp.ToXMVECTOR(), size ), ( axisA - cameraP ).ToXMVECTOR(), 2 * size, 2, DirectX::Colors::Gray / 2 );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

				if ( mViewportData.mIsActive ) ImGui::SetTooltip(
					"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
					mViewportData.mWorldMouseU, mViewportData.mWorldMouseV,
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
				DX::DrawGrid( mWireframeGridBatch.get(), DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( diffP2 + axisA - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
				DX::DrawGrid( mWireframeGridBatch.get(), DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( axisA - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

				if ( mViewportData.mIsActive ) ImGui::SetTooltip(
					"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
					mViewportData.mWorldMouseU, mViewportData.mWorldMouseV,
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
				DX::DrawGrid( mWireframeGridBatch.get(), DirectX::XMVectorScale( axisDir1.ToXMVECTOR(), size ), DirectX::XMVectorScale( axisDir2.ToXMVECTOR(), size ), ( axisA - cameraP ).ToXMVECTOR(), 2 * size, 2 * size, DirectX::Colors::Gray / 2 );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( mousePos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( objPos3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );
				DX::Draw( mWireframeGridBatch.get(), DirectX::BoundingBox( objPosNew3, DirectX::XMFLOAT3( .1, .1, .1 ) ) );

				if ( mViewportData.mIsActive ) ImGui::SetTooltip(
					"MouseClip=(%.2f, %.2f)\nMouse3D=(%.2f, %.2f, %.2f)\nDot=(%.2f)\nPlaneVT=(%.2f)\nMouseDir=(%.2f, %.2f, %.2f)\nreprojectedMousePos=(%.2f, %.2f, %.2f, %.2f)",
					mViewportData.mWorldMouseU, mViewportData.mWorldMouseV,
					mousePos.GetX(),
					mousePos.GetY(),
					mousePos.GetZ(),
					flip,
					planeVT,
					mouseDir.GetX(), mouseDir.GetY(), mouseDir.GetZ(),
					reprojectedMousePos.x, reprojectedMousePos.y, reprojectedMousePos.z, reprojectedMousePos.w
				);
			}

			mWireframeGridBatch->End();
		}
	}

	{
		mWireframeBoxShader->Apply( inDeviceContext );
		mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
		mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

		const auto &selectionContext = inLevelInterface->GetSelectionCtx();
		const auto &entityManager = inLevelInterface->GetEntityManager();

		// Iterate over all entities
		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		{
			auto view = cregistry.view<EntityType, Position, BoundingBox, entt::tag<"is_visible"_hs>>( entt::exclude<entt::tag<"is_selected"_hs>> );
			//view.use<BoundingBox>();
			for ( const entt::entity entity : view ) {
				const auto &entityType = view.get<EntityType>( entity );
				const auto &position = view.get<Position>( entity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

				uint32_t entityColorU32 = entityManager.GetEntityTypeColor( entityType );

				DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

				Vector4D rebasedEntityPosition = ( position - perspectiveContext.mCenter3D );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}

		inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );

		{
			auto view = cregistry.view<Position, BoundingBox, entt::tag<"is_visible"_hs>, entt::tag<"is_selected"_hs>>();

			entt::entity selectedEntity = selectionContext.GetSelectedEntity();

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 128, 0, 255 ) );
			for ( const entt::entity entity : view ) {
				if ( selectedEntity == entity ) continue;

				const auto &position = view.get<Position>( entity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

				Vector4D rebasedEntityPosition = ( position - perspectiveContext.mCenter3D );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
			}

			entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 255, 0, 255 ) );
			if ( selectedEntity != entt::null ) {
				const auto &position = view.get<Position>( selectedEntity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( selectedEntity ).mValue;

				Vector4D rebasedEntityPosition = ( position - perspectiveContext.mCenter3D );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}
	}

	Resolve( inDeviceContext );
}
