#include "pch.h"
#include "Cyclone/UI/ViewportManager.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

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

void Cyclone::UI::ViewportManager::ToolbarUpdate()
{
	ImVec2 viewSize = ImGui::GetWindowSize();

	ImGui::PushStyleVarX( ImGuiStyleVar_SelectableTextAlign, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 4.0f );

	const float textOffset = 6.0f;
	const float itemOffset = 3.0f;

	const float textSpacing = 5.0f;
	const float childSpacing = 8.0f;

	ImGui::SetCursorPos( { 5, 5 } );

	ImGui::BeginChild( "##GridSizeChunk", { 0, viewSize.y - 10 }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_FrameStyle );
	{
		ImGui::SetCursorPosY( textOffset );
		ImGui::Text( "Grid:" );

		ImGui::SameLine( 0.0f, textSpacing );

		ImGui::SetCursorPosY( itemOffset );
		ImGui::SetNextItemWidth( 52.0f );
		std::string subGridLevelPreview = mGridContext.kGridSizeText[mGridContext.mGridSizeIndex];
		if ( ImGui::BeginCombo( "##SubGridLevel", subGridLevelPreview.c_str(), ImGuiComboFlags_HeightLarge ) ) {
			for ( int i = 0; i < std::size( mGridContext.kGridSizes ); ++i ) {
				const bool isSelected = mGridContext.mGridSizeIndex == i;
				if ( ImGui::Selectable( mGridContext.kGridSizeText[i], isSelected ) ) {
					mGridContext.SetGridSize( i );
				}
				if ( isSelected ) ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		// Adjust grid size with [ and ]
		if ( ImGui::IsKeyPressed( ImGuiKey_LeftBracket, false ) ) mGridContext.SetGridSize( mGridContext.mGridSizeIndex - 1 );
		if ( ImGui::IsKeyPressed( ImGuiKey_RightBracket, false ) ) mGridContext.SetGridSize( mGridContext.mGridSizeIndex + 1 );
	}
	ImGui::EndChild();

	ImGui::SameLine( 0.0f, childSpacing );

	ImGui::BeginChild( "##GridSnapChunk", { 0, viewSize.y - 10 }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_FrameStyle );
	{
		ImGui::SetCursorPosY( textOffset );
		ImGui::Text( "Snap:" );

		ImGui::SameLine( 0.0f, textSpacing );

		ImGui::SetCursorPosY( itemOffset );
		if ( ImGui::RadioButton( "To Grid", mGridContext.mSnapType == ViewportGridContext::ESnapType::ToGrid ) ) mGridContext.mSnapType = ViewportGridContext::ESnapType::ToGrid;

		ImGui::SameLine();

		ImGui::SetCursorPosY( itemOffset );
		if ( ImGui::RadioButton( "By Grid", mGridContext.mSnapType == ViewportGridContext::ESnapType::ByGrid ) ) mGridContext.mSnapType = ViewportGridContext::ESnapType::ByGrid;

		ImGui::SameLine();

		ImGui::SetCursorPosY( itemOffset );
		if ( ImGui::RadioButton( "None ", mGridContext.mSnapType == ViewportGridContext::ESnapType::None ) ) mGridContext.mSnapType = ViewportGridContext::ESnapType::None;
		
	}
	ImGui::EndChild();

	ImGui::PopStyleVar( 2 );

	
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	ImVec2 perspectiveViewSize;

	if ( mShouldAutosize || ImGui::IsKeyChordPressed( ImGuiKey_A | ImGuiMod_Ctrl ) ) {
		mShouldAutosize = false;
		ImGui::SetNextWindowSize( { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 } );
	}

	ImGui::SetNextWindowSizeConstraints( { 64.0f, 64.0f }, { ImGui::GetContentRegionAvail().x - 64.0f, ImGui::GetContentRegionAvail().y - 64.0f } );
	if ( ImGui::BeginChild( "PerspectiveView", ImVec2( ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
		perspectiveViewSize = ImGui::GetWindowSize();
		mViewportPerspective->Update( inDeltaTime, inLevelInterface, mGridContext, mPerspectiveContext );
		DrawViewportOverlay( "Perspective" );
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, perspectiveViewSize.y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		mViewportTop->Update( inDeltaTime, inLevelInterface, mGridContext, mOrthographicContext );
		DrawViewportOverlay( "Top (X/Z)" );
	}
	ImGui::EndChild();

	if ( ImGui::BeginChild( "FrontView", ImVec2( perspectiveViewSize.x, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		mViewportFront->Update( inDeltaTime, inLevelInterface, mGridContext, mOrthographicContext );
		DrawViewportOverlay( "Front (X/Y)" );
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
		mViewportSide->Update( inDeltaTime, inLevelInterface, mGridContext, mOrthographicContext );
		DrawViewportOverlay( "Side (Y/Z)" );
	}
	ImGui::EndChild();

	mOrthographicContext.mCenter2D = Vector4D::sClamp( mOrthographicContext.mCenter2D, Vector4D::sReplicate( -mGridContext.mWorldLimit ), Vector4D::sReplicate( mGridContext.mWorldLimit ) );
}

void Cyclone::UI::ViewportManager::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	mViewportPerspective->Render( inDeviceContext, inLevelInterface, mGridContext, mPerspectiveContext );
	mViewportTop->Render( inDeviceContext, inLevelInterface, mGridContext, mOrthographicContext );
	mViewportFront->Render( inDeviceContext, inLevelInterface, mGridContext, mOrthographicContext );
	mViewportSide->Render( inDeviceContext, inLevelInterface, mGridContext, mOrthographicContext );
}