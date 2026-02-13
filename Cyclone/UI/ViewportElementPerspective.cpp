#include "pch.h"
#include "Cyclone/UI/ViewportElementPerspective.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

using Cyclone::Math::Vector4D;

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

void Cyclone::UI::ViewportElementPerspective::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, ViewportGridContext &inGridContext, ViewportPerspectiveContext &inPerspectiveContext )
{
	ImVec2 viewSize = ImGui::GetWindowSize();
	ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::Image( GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	const bool isHovered = ImGui::IsItemHovered();
	const bool isActive = ImGui::IsItemActive();

	if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
		inPerspectiveContext.mCameraPitch += io.MouseDelta.y * kMouseSensitivity;
		inPerspectiveContext.mCameraYaw -= io.MouseDelta.x * kMouseSensitivity;

		constexpr float pitchLimit = DirectX::XM_PIDIV2 - 0.01f;
		inPerspectiveContext.mCameraPitch = std::clamp( inPerspectiveContext.mCameraPitch, -pitchLimit, pitchLimit );
		inPerspectiveContext.mCameraYaw = inPerspectiveContext.mCameraYaw - DirectX::XM_2PI * std::floor( inPerspectiveContext.mCameraYaw / DirectX::XM_2PI );

		float forward = 0.0f;
		float left = 0.0f;

		forward += ImGui::IsKeyDown( ImGuiKey_W );
		forward -= ImGui::IsKeyDown( ImGuiKey_S );
		left += ImGui::IsKeyDown( ImGuiKey_A );
		left -= ImGui::IsKeyDown( ImGuiKey_D );

		forward *= kKeyboardSensitivity * inDeltaTime;
		left *= kKeyboardSensitivity * inDeltaTime;

		if ( forward || left ) {
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw( inPerspectiveContext.mCameraPitch, inPerspectiveContext.mCameraYaw, 0.0f );
			inPerspectiveContext.mCenter3D += Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::XMVectorSet( left, 0, forward, 0 ), rotationMatrix ) );
		}
	}

	if ( isHovered ) {
		float scroll = io.MouseWheel;
		scroll *= kCameraDollySensitivity;
		if ( scroll ) {
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw( inPerspectiveContext.mCameraPitch, inPerspectiveContext.mCameraYaw, 0.0f );
			inPerspectiveContext.mCenter3D += Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::XMVectorSet( 0, 0, scroll, 0 ), rotationMatrix ) );
		}
	}
}

void Cyclone::UI::ViewportElementPerspective::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportPerspectiveContext &inPerspectiveContext )
{
	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix( inPerspectiveContext.mCameraPitch, inPerspectiveContext.mCameraYaw ), GetProjMatrix( mWidth, mHeight, kHorizontalFOV, inGridContext.mWorldLimit ) );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		mWireframeGridBatch->DrawLine(
			{ ( -inPerspectiveContext.mCenter3D ).ToXMVECTOR(), DirectX::Colors::DarkRed },
			{ ( -inPerspectiveContext.mCenter3D + Vector4D::sZeroSetValueByIndex<0>( 1 ) ).ToXMVECTOR(), DirectX::Colors::DarkRed }
		);

		mWireframeGridBatch->DrawLine(
			{ ( -inPerspectiveContext.mCenter3D ).ToXMVECTOR(), DirectX::Colors::Green },
			{ ( -inPerspectiveContext.mCenter3D + Vector4D::sZeroSetValueByIndex<1>( 1 ) ).ToXMVECTOR(), DirectX::Colors::Green }
		);

		mWireframeGridBatch->DrawLine(
			{ ( -inPerspectiveContext.mCenter3D ).ToXMVECTOR(), DirectX::Colors::DarkBlue },
			{ ( -inPerspectiveContext.mCenter3D + Vector4D::sZeroSetValueByIndex<2>( 1 ) ).ToXMVECTOR(), DirectX::Colors::DarkBlue }
		);
	}
	mWireframeGridBatch->End();

	// Switch to depth buffer
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthDefault(), 0 );

	mWireframeGridBatch->Begin();
	{
		// Iterate over all entities
		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>();
		for ( const entt::entity entity : view ) {

			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
			const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity );

			bool entityInSelection = inLevelInterface->GetSelectedEntities().contains( entity );
			bool entityIsSelected = inLevelInterface->GetSelectedEntity() == entity;

			uint32_t entityColorU32;
			if ( entityIsSelected ) {
				entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
			}
			else if ( entityInSelection ) {
				entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
			}
			else {
				entityColorU32 = entt::resolve( static_cast<entt::id_type>( entityType ) ).data( "debug_color"_hs ).get( {} ).cast<uint32_t>();
			}

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

			Vector4D rebasedEntityPosition = ( position - inPerspectiveContext.mCenter3D );
			Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			Cyclone::Util::RenderWireframeBoundingBox( mWireframeGridBatch.get(), rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}
	}
	mWireframeGridBatch->End();

	Resolve( inDeviceContext );
}
