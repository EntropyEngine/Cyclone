#include "pch.h"

#include "Cyclone/UI/ViewportManager.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui_internal.h>

Cyclone::UI::ViewportManager::ViewportManager( ID3D11Device3 *inDevice )
{
	mViewportPerspective = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::CornflowerBlue );
	mViewportTop = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportFront = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportSide = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime )
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	ImVec2 perspectiveViewSize;

	ImGui::SetNextWindowSizeConstraints( { 64.0f, 64.0f }, { ImGui::GetContentRegionAvail().x - 64.0f, ImGui::GetContentRegionAvail().y - 64.0f } );
	if ( ImGui::BeginChild( "PerspectiveView", ImVec2( ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
		perspectiveViewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportPerspective->GetImageSRV( static_cast<size_t>( perspectiveViewSize.x ), static_cast<size_t>( perspectiveViewSize.y ) ), perspectiveViewSize );

		ImGui::SetCursorPos( { 0, 0 } );
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1 = ImGui::CalcTextSize( "Perspective" );
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( p0, { p0.x + p1.x + 8, p0.y + p1.y + 8 }, IM_COL32( 0, 0, 0, 64 ) );

		ImGui::SetCursorPos( { 4, 4 } );
		ImGui::Text( "Perspective" );

		ImGui::EndChild();
	}

	ImGui::SameLine();
	if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, perspectiveViewSize.y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportTop->GetImageSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

		ImGui::SetCursorPos( { 0, 0 } );
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1 = ImGui::CalcTextSize( "Top (X/Z)" );
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( p0, { p0.x + p1.x + 8, p0.y + p1.y + 8 }, IM_COL32( 0, 0, 0, 64 ) );

		ImGui::SetCursorPos( { 4, 4 } );
		ImGui::Text( "Top (X/Y)" );

		ImGui::EndChild();
	}

	if ( ImGui::BeginChild( "FrontView", ImVec2( perspectiveViewSize.x, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportFront->GetImageSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

		ImGui::SetCursorPos( { 0, 0 } );
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1 = ImGui::CalcTextSize( "Front (X/Y)" );
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( p0, { p0.x + p1.x + 8, p0.y + p1.y + 8 }, IM_COL32( 0, 0, 0, 64 ) );

		ImGui::SetCursorPos( { 4, 4 } );
		ImGui::Text( "Front (X/Y)" );

		ImGui::EndChild();
	}

	ImGui::SameLine();
	if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportSide->GetImageSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

		ImGui::SetCursorPos( { 0, 0 } );
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1 = ImGui::CalcTextSize( "Side (Y/Z)" );
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( p0, { p0.x + p1.x + 8, p0.y + p1.y + 8 }, IM_COL32( 0, 0, 0, 64 ) );

		ImGui::SetCursorPos( { 4, 4 } );
		ImGui::Text( "Side (Y/Z)" );

		ImGui::EndChild();
	}
}