#include "pch.h"
#include "Cyclone/UI/ViewportManager.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// STL Includes
#include <format>

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

// DX Includes
#include <DirectXHelpers.h>

using Cyclone::Math::Vector4D;

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

Cyclone::UI::ViewportManager::ViewportManager()
{
	mViewportPerspective = std::make_unique<ViewportElementPerspective>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportTop = std::make_unique<ViewportElementOrthographic<EViewportType::TopXZ>>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportFront = std::make_unique<ViewportElementOrthographic<EViewportType::FrontXY>>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportSide = std::make_unique<ViewportElementOrthographic<EViewportType::SideYZ>>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
}

void Cyclone::UI::ViewportManager::SetDevice( ID3D11Device3 *inDevice )
{
	mViewportPerspective->SetDevice( inDevice );
	mViewportTop->SetDevice( inDevice );
	mViewportFront->SetDevice( inDevice );
	mViewportSide->SetDevice( inDevice );
}

void Cyclone::UI::ViewportManager::MenuBarUpdate()
{
	if ( ImGui::MenuItem( "Autosize Viewports", "Ctrl+A") ) mShouldAutosize = true;
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	ImVec2 perspectiveViewSize;

	ImVec2 viewSize = ImGui::GetWindowSize();

	if ( mShouldAutosize || ImGui::IsKeyChordPressed( ImGuiKey_A | ImGuiMod_Ctrl ) ) {
		mShouldAutosize = false;
		ImGui::SetNextWindowSize( { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 } );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, { kMinViewportSize, kMinViewportSize } );

	ImGui::SetNextWindowSizeConstraints( { kMinViewportSize, kMinViewportSize }, { viewSize.x - kMinViewportSize, viewSize.y - kMinViewportSize } );
	if ( ImGui::BeginChild( "PerspectiveView", { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
		perspectiveViewSize = ImGui::GetWindowSize();
		mViewportPerspective->Update( inDeltaTime, inLevelInterface );
		DrawViewportOverlay( "Perspective" );
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, perspectiveViewSize.y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		mViewportTop->Update( inDeltaTime, inLevelInterface );
		DrawViewportOverlay( "Top (X/Z)" );
	}
	ImGui::EndChild();

	if ( ImGui::BeginChild( "FrontView", ImVec2( perspectiveViewSize.x, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		mViewportFront->Update( inDeltaTime, inLevelInterface );
		DrawViewportOverlay( "Front (X/Y)" );
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
		mViewportSide->Update( inDeltaTime, inLevelInterface );
		DrawViewportOverlay( "Side (Y/Z)" );
	}
	ImGui::EndChild();

	ImGui::PopStyleVar( 1 );

	const auto &gridContext = inLevelInterface->GetGridCtx();
	auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	orthographicContext.mCenter2D = Vector4D::sClamp( orthographicContext.mCenter2D, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );
}

void Cyclone::UI::ViewportManager::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	mViewportPerspective->Render( inDeviceContext, inLevelInterface );
	mViewportTop->Render( inDeviceContext, inLevelInterface );
	mViewportFront->Render( inDeviceContext, inLevelInterface );
	mViewportSide->Render( inDeviceContext, inLevelInterface );
}