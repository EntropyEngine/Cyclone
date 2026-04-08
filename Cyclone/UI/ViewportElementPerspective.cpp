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

	mViewportData.mAbsoluteMouse = viewportAbsMousePos;

	mViewportData.mWorldMouseU = viewportRelMousePos.x / ( viewSize.x / 2.0f );
	mViewportData.mWorldMouseV = -viewportRelMousePos.y / ( viewSize.y / 2.0f );

	const auto &gridContext = inLevelInterface->GetGridCtx();

	// Update matrices
	mViewportData.mViewMatrix = GetViewMatrix( perspectiveContext.mCameraPitch, perspectiveContext.mCameraYaw );
	mViewportData.mProjMatrix = GetProjMatrix( mWidth, mHeight, kHorizontalFOV, gridContext.mWorldLimit );
}

void Cyclone::UI::ViewportElementPerspective::UpdateTools( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const Tool::ToolChanger &inTools )
{
	for ( auto &tool : inTools.mTools ) {
		tool->OnUpdate( EViewportType::Perspective, inLevelInterface, mViewportData );
	}
}

void Cyclone::UI::ViewportElementPerspective::DrawGizmos( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const Tool::ToolChanger &inTools )
{
	for ( auto &tool : inTools.mTools ) {
		tool->OnDraw( EViewportType::Perspective, inLevelInterface, mViewportData );
	}

	mViewportData.mDrawList->ChannelsMerge();
}

void Cyclone::UI::ViewportElementPerspective::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const Tool::ToolChanger &inTools )
{

	Clear( inDeviceContext );

	ViewportElement::Render<EViewportType::Perspective>( inDeviceContext, inLevelInterface, inTools );

	Resolve( inDeviceContext );
}
