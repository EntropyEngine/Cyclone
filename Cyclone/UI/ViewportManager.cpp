#include "pch.h"

#include "Cyclone/UI/ViewportManager.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui_internal.h>

namespace
{
	void DrawViewportOverlay( const char *inText, float inPadding = 4 )
	{
		ImGui::SetCursorPos( { 0, 0 } );
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1 = ImGui::CalcTextSize( inText );
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( p0, { p0.x + p1.x + inPadding * 2, p0.y + p1.y + inPadding * 2 }, IM_COL32( 0, 0, 0, 128 ) );

		ImGui::SetCursorPos( { inPadding, inPadding } );
		ImGui::Text( inText );
	}
}

Cyclone::UI::ViewportManager::ViewportManager( ID3D11Device3 *inDevice )
{
	mViewportPerspective = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::CornflowerBlue );
	mViewportTop = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportFront = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportSide = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime )
{
	ImGuiIO &io = ImGui::GetIO();

	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	ImVec2 perspectiveViewSize;

	ImGui::SetNextWindowSizeConstraints( { 64.0f, 64.0f }, { ImGui::GetContentRegionAvail().x - 64.0f, ImGui::GetContentRegionAvail().y - 64.0f } );
	if ( ImGui::BeginChild( "PerspectiveView", ImVec2( ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
		perspectiveViewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportPerspective->GetImageSRV( static_cast<size_t>( perspectiveViewSize.x ), static_cast<size_t>( perspectiveViewSize.y ) ), perspectiveViewSize );

		DrawViewportOverlay( "Perspective" );

		ImGui::EndChild();
	}

	ImGui::SameLine();
	if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, perspectiveViewSize.y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

		ImGui::SetCursorPos( { 0, 0 } );
		ImGui::Image( mViewportTop->GetImageSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
		{
			ImGui::SetCursorPos( { 0, 0 } );
			ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();

			ImVec2 viewportAbsPos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
			ImVec2 viewportRelPos( viewportAbsPos.x - viewSize.x / 2.0f, viewportAbsPos.y - viewSize.y / 2.0f );

			double xPos = mCenterX2D - viewportRelPos.x * mZoomScale2D;
			double zPos = mCenterZ2D - viewportRelPos.y * mZoomScale2D;

			if ( isHovered ) ImGui::SetTooltip(
				"Mouse pos: (%.0f, %.0f)\n"
				"Viewport abs pos: (%.0f, %.0f)\n"
				"Viewport rel pos: (%.1f, %.1f)\n"
				"World Pos: (x=%.1f, z=%.1f)",
				io.MousePos.x, io.MousePos.y,
				viewportAbsPos.x, viewportAbsPos.y,
				viewportRelPos.x, viewportRelPos.y,
				xPos, zPos
			);
		}
		ImGui::PopStyleVar( 2 );

		DrawViewportOverlay( "Top (X/Z)" );

		ImGui::EndChild();
	}

	if ( ImGui::BeginChild( "FrontView", ImVec2( perspectiveViewSize.x, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

		ImGui::SetCursorPos( { 0, 0 } );
		ImGui::Image( mViewportFront->GetImageSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
		{
			ImGui::SetCursorPos( { 0, 0 } );
			ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();

			ImVec2 viewportAbsPos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
			ImVec2 viewportRelPos( viewportAbsPos.x - viewSize.x / 2.0f, viewportAbsPos.y - viewSize.y / 2.0f );

			double xPos = mCenterX2D - viewportRelPos.x * mZoomScale2D;
			double yPos = mCenterY2D - viewportRelPos.y * mZoomScale2D;

			if ( isHovered ) ImGui::SetTooltip(
				"Mouse pos: (%.0f, %.0f)\n"
				"Viewport abs pos: (%.0f, %.0f)\n"
				"Viewport rel pos: (%.1f, %.1f)\n"
				"World Pos: (x=%.1f, y=%.1f)",
				io.MousePos.x, io.MousePos.y,
				viewportAbsPos.x, viewportAbsPos.y,
				viewportRelPos.x, viewportRelPos.y,
				xPos, yPos
			);
		}
		ImGui::PopStyleVar( 2 );

		DrawViewportOverlay( "Front (X/Y)" );

		ImGui::EndChild();
	}

	ImGui::SameLine();
	if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

		ImGui::SetCursorPos( { 0, 0 } );
		ImGui::Image( mViewportSide->GetImageSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
		{
			ImGui::SetCursorPos( { 0, 0 } );
			ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
			const bool isHovered = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();

			ImVec2 viewportAbsPos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
			ImVec2 viewportRelPos( viewportAbsPos.x - viewSize.x / 2.0f, viewportAbsPos.y - viewSize.y / 2.0f );

			double yPos = mCenterY2D - viewportRelPos.y * mZoomScale2D;
			double zPos = mCenterZ2D - viewportRelPos.x * mZoomScale2D;

			if ( isHovered ) ImGui::SetTooltip(
				"Mouse pos: (%.0f, %.0f)\n"
				"Viewport abs pos: (%.0f, %.0f)\n"
				"Viewport rel pos: (%.1f, %.1f)\n"
				"World Pos: (z=%.1f, y=%.1f)",
				io.MousePos.x, io.MousePos.y,
				viewportAbsPos.x, viewportAbsPos.y,
				viewportRelPos.x, viewportRelPos.y,
				zPos, yPos
			);
		}
		ImGui::PopStyleVar( 2 );

		DrawViewportOverlay( "Side (Y/Z)" );

		ImGui::EndChild();
	}
}

void Cyclone::UI::ViewportManager::RenderPerspective( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportPerspective->Clear( inDeviceContext );
	// Render
	mViewportPerspective->Resolve( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderTop( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportTop->Clear( inDeviceContext );
	// Render
	mViewportTop->Resolve( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderFront( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportFront->Clear( inDeviceContext );
	// Render
	mViewportFront->Resolve( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderSide( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportSide->Clear( inDeviceContext );
	// Render
	mViewportSide->Resolve( inDeviceContext );
}
