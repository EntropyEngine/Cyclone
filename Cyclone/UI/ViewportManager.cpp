#include "pch.h"
#include "Cyclone/UI/ViewportManager.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

// STL Includes
#include <format>

// ImGui Includes
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
	mViewportPerspective = std::make_unique<ViewportElementPerspective>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black, mAntialiasingEnabled );
	mViewportTop = std::make_unique<ViewportElementOrthographic<EViewportType::TopXZ>>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black, mAntialiasingEnabled );
	mViewportFront = std::make_unique<ViewportElementOrthographic<EViewportType::FrontXY>>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black, mAntialiasingEnabled );
	mViewportSide = std::make_unique<ViewportElementOrthographic<EViewportType::SideYZ>>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black, mAntialiasingEnabled );
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
	if ( ImGui::MenuItem( "Enable Antialiasing", nullptr, &mAntialiasingEnabled ) ) ToggleAntialiasing( mAntialiasingEnabled );
	ImGui::Separator();
	if ( ImGui::MenuItem( "Autosize Viewports", "Ctrl+A") ) mShouldAutosize = true;
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
	ImVec2 viewSizePerspective, viewSizeTop, viewSizeFront, viewSizeSide;
	ImVec2 viewSize = ImGui::GetWindowSize();

	const auto &entityManager = inLevelInterface->GetEntityManager();
	entt::registry &registry = inLevelInterface->GetRegistry();

	registry.clear<entt::tag<"is_visible"_hs>, entt::tag<"draw_perspective"_hs>, entt::tag<"draw_top"_hs>, entt::tag<"draw_front"_hs>, entt::tag<"draw_side"_hs>>();
	auto view = registry.group<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::EntityCategory, Cyclone::Core::Component::Visible, Cyclone::Core::Component::Selectable>();
	for ( const entt::entity entity : view ) {
		const auto &entityCategory = view.get<Cyclone::Core::Component::EntityCategory>( entity );
		if ( !entityManager.GetEntityCategoryIsVisible( entityCategory ) ) continue;

		const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
		if ( !entityManager.GetEntityTypeIsVisible( entityType ) ) continue;

		if ( !static_cast<bool>( view.get<Cyclone::Core::Component::Visible>( entity ) ) ) continue;

		registry.emplace<entt::tag<"is_visible"_hs>>( entity );
	}

	// Instantiate all windows and grab their positional data
	{
		if ( mShouldAutosize ) {
			mShouldAutosize = false;
			ImGui::SetNextWindowSize( { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 } );
		}

		ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, { kMinViewportSize, kMinViewportSize } );

		ImGui::SetNextWindowSizeConstraints( { kMinViewportSize, kMinViewportSize }, { viewSize.x - kMinViewportSize, viewSize.y - kMinViewportSize } );
		if ( ImGui::BeginChild( "PerspectiveView", { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
			viewSizePerspective = ImGui::GetWindowSize();
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if ( ImGui::BeginChild( "TopView", { ImGui::GetContentRegionAvail().x, viewSizePerspective.y }, ImGuiChildFlags_Borders, viewportFlags ) ) {
			viewSizeTop = ImGui::GetWindowSize();
		}
		ImGui::EndChild();

		if ( ImGui::BeginChild( "FrontView", { viewSizePerspective.x, ImGui::GetContentRegionAvail().y }, ImGuiChildFlags_Borders, viewportFlags ) ) {
			viewSizeFront = ImGui::GetWindowSize();
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
			viewSizeSide = ImGui::GetWindowSize();
		}
		ImGui::EndChild();

		ImGui::PopStyleVar( 1 );
	}

	// Perform actual updates
	{
		ImGui::SetNextWindowSizeConstraints( viewSizePerspective, viewSizePerspective );
		if ( ImGui::BeginChild( "PerspectiveView", viewSizePerspective ) ) {
			mViewportPerspective->Update( inDeltaTime, inLevelInterface );
			DrawViewportOverlay( "Perspective" );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeTop, viewSizeTop );
		if ( ImGui::BeginChild( "TopView", viewSizeTop ) ) {
			mViewportTop->Update( inDeltaTime, inLevelInterface );
			DrawViewportOverlay( "Top (X/Z)" );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeFront, viewSizeFront );
		if ( ImGui::BeginChild( "FrontView", viewSizeFront ) ) {
			mViewportFront->Update( inDeltaTime, inLevelInterface );
			DrawViewportOverlay( "Front (X/Y)" );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeSide, viewSizeSide );
		if ( ImGui::BeginChild( "SideView", viewSizeSide ) ) {
			mViewportSide->Update( inDeltaTime, inLevelInterface );
			DrawViewportOverlay( "Side (Y/Z)" );
		}
		ImGui::EndChild();
	}

	const auto &gridContext = inLevelInterface->GetGridCtx();
	auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	orthographicContext.mCenter2D = Vector4D::sClamp( orthographicContext.mCenter2D, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );
}

void Cyclone::UI::ViewportManager::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface )
{
	mViewportPerspective->Render( inDeviceContext, inLevelInterface );
	mViewportTop->Render( inDeviceContext, inLevelInterface );
	mViewportFront->Render( inDeviceContext, inLevelInterface );
	mViewportSide->Render( inDeviceContext, inLevelInterface );
}

void Cyclone::UI::ViewportManager::ToggleAntialiasing( bool inEnabled )
{
	mAntialiasingEnabled = inEnabled;

	mViewportPerspective->ToggleAntialiasing( inEnabled );
	mViewportTop->ToggleAntialiasing( inEnabled );
	mViewportFront->ToggleAntialiasing( inEnabled );
	mViewportSide->ToggleAntialiasing( inEnabled );
}