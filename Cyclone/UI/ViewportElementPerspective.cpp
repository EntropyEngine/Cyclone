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
}

void Cyclone::UI::ViewportElementPerspective::UpdateTools( float inDeltaTime, Cyclone::Core::LevelInterface * inLevelInterface )
{}

void Cyclone::UI::ViewportElementPerspective::DrawGizmos( float inDeltaTime, Cyclone::Core::LevelInterface * inLevelInterface )
{}

void Cyclone::UI::ViewportElementPerspective::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();

	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mCommonStates->Wireframe() : mWireframeRSS.Get() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	DirectX::XMMATRIX viewMatrix = GetViewMatrix( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw );
	DirectX::XMMATRIX projMatrix = GetProjMatrix( mWidth, mHeight, kHorizontalFOV, gridContext.mWorldLimit );

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

	// Gizmo tests
	{
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
