#include "pch.h"
#include "Cyclone/UI/ViewportElementPerspective.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"

// Cyclone math
#include "Cyclone/Math/Matrix.hpp"

// ImGui Includes
#include <imgui_internal.h>

// ImGuizmo
#include <ImGuizmo/ImGuizmo.h>

using Cyclone::Math::Vector4D;
using Cyclone::Math::Matrix44D;

using Cyclone::Core::Component::EntityType;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Component::Rotation;
using Cyclone::Core::Component::BoundingBox;
using Cyclone::Core::Component::PathTag;
using Cyclone::Core::Component::PathData;

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

	if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) && !ImGuizmo::IsUsingAny() ) {
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

		ImGui::SetKeyOwner( ImGuiKey_F24, mViewportData.mCanvasID, ImGuiInputFlags_LockThisFrame );
	}

	if ( mViewportData.mIsActive && !ImGuizmo::IsUsingAny() ) {
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

	mViewportData.mDrawList->ChannelsMerge();
}

void Cyclone::UI::ViewportElementPerspective::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	using DrawTag = ViewportTypeTraits<EViewportType::Perspective>::DrawTag;

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityManager = inLevelInterface->GetEntityManager();

	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();

	const entt::registry &cregistry = inLevelInterface->GetRegistry();

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

	// Render paths
	{
		// Iterate over all entities
		{
			mWireframeGridBatch->Begin();

			auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathData, DrawTag>();
			//view.use<BoundingBox>();
			for ( const entt::entity entity : view ) {
				const auto &entityType = view.get<EntityType>( entity );
				const auto &position = view.get<Position>( entity ).mValue;
				const auto &rotation = view.get<Rotation>( entity ).mPitchYawRoll;
				const auto &pathData = view.get<PathData>( entity );

				Matrix44D rotmat = Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( rotation ) );
				Vector4D rebasedEntityPosition = ( position - perspectiveContext.mCenter3D );

				uint32_t entityColorU32;
				if ( entity == selectedEntity ) {
					entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
				}
				else if ( selectedEntities.contains( entity ) ) {
					entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
				}
				else {
					entityColorU32 = entityManager.GetEntityTypeColor( entityType );
				}
				DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

				std::vector<DirectX::VertexPositionColor> linePoints( ( pathData.mKnots.size() - 1 ) * 65 );
				//std::vector<DirectX::VertexPositionColor> linePointsL( ( pathData.mKnots.size() - 1 ) * 17 );
				//std::vector<DirectX::VertexPositionColor> linePointsR( ( pathData.mKnots.size() - 1 ) * 17 );
				for ( size_t s = 0; s + 1 < pathData.mKnots.size(); ++s ) {
					for ( size_t i = 0; i <= 64; ++i ) {
						float u = static_cast<float>( i ) / 64;
						DirectX::XMStoreFloat3( &linePoints[s * 65 + i].position, ( rotmat.TransformCoord3Unit( pathData.Interpolate( s, u ) ) + rebasedEntityPosition ).ToXMVECTOR() );
						DirectX::XMStoreFloat4( &linePoints[s * 65 + i].color, entityColorV );

						if ( i % 4 == 0 ) {
							using namespace DirectX;

							//DirectX::XMStoreFloat3( &linePointsL[s * 17 + i / 4].position, ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, 0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR() );
							//DirectX::XMStoreFloat4( &linePointsL[s * 17 + i / 4].color, entityColorV * 0.75f );

							//DirectX::XMStoreFloat3( &linePointsR[s * 17 + i / 4].position, ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, -0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR() );
							//DirectX::XMStoreFloat4( &linePointsR[s * 17 + i / 4].color, entityColorV * 0.75f );

							
							DirectX::XMVECTOR A = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, -0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR();
							DirectX::XMVECTOR B = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, 0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR();
							DirectX::XMVECTOR C = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, 0.5, -0.1 ) ) + rebasedEntityPosition ).ToXMVECTOR();
							DirectX::XMVECTOR D = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, -0.5, -0.1 ) ) + rebasedEntityPosition ).ToXMVECTOR();

							mWireframeGridBatch->DrawLine( { A, entityColorV }, { B, entityColorV } );
							mWireframeGridBatch->DrawLine( { B, entityColorV }, { C, entityColorV } );
							mWireframeGridBatch->DrawLine( { C, entityColorV }, { D, entityColorV } );
							mWireframeGridBatch->DrawLine( { D, entityColorV }, { A, entityColorV } );
							

							//mWireframeGridBatch->DrawLine( linePointsL[s * 17 + i / 4], linePointsR[s * 17 + i / 4] );
						}

					}
				}

				mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePoints.data(), linePoints.size() );
				//mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsL.data(), linePointsL.size() );
				//mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsR.data(), linePointsR.size() );
				
				// TODO
				// Draw the path
				// TODO
			}

			mWireframeGridBatch->End();
		}
	}

	// Call all tool renders with depth enabled
	{
		inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mToolRSS_MSAA.Get() : mToolRSS_NOAA.Get() );

		mWireframeGridBatch->Begin();
		for ( auto &tool : inTools ) {
			tool->OnRender( EViewportType::Perspective, inLevelInterface, mViewportData, mWireframeGridBatch.get() );
		}
		mWireframeGridBatch->End();

	}

	// Render bounding boxes (excluding paths)
	{
		inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mCommonStates->Wireframe() : mWireframeRSS.Get() );
		//inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mToolRSS_MSAA.Get() : mToolRSS_NOAA.Get() );

		mWireframeBoxShader->Apply( inDeviceContext );
		mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
		mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

		{
			auto view = cregistry.view<EntityType, Position, BoundingBox, DrawTag>( entt::exclude<entt::tag<"is_selected"_hs>, PathTag> );
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
			auto view = cregistry.view<Position, BoundingBox, DrawTag, entt::tag<"is_selected"_hs>>( entt::exclude<PathTag> );

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
			if ( view.contains( selectedEntity ) ) {
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
