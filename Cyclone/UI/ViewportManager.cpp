#include "pch.h"
#include "Cyclone/UI/ViewportManager.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone UI tools
#include "Cyclone/UI/Tool/SelectionTool.hpp"
#include "Cyclone/UI/Tool/SelectionTransformTool.hpp"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"
#include "Cyclone/UI/Tool/PathSelectionTool.hpp"
#include "Cyclone/UI/Tool/MeshSelectionTool.hpp"

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

using Cyclone::Core::Component::EntityType;
using Cyclone::Core::Component::EntityCategory;
using Cyclone::Core::Component::Visible;
using Cyclone::Core::Component::Selectable;

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
	const DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
	const DirectX::XMVECTORF32 clearColor = DirectX::Colors::Black;

	mViewportPerspective = std::make_unique<ViewportElementPerspective>( rtvFormat, dsvFormat, clearColor, mAntialiasingEnabled );
	mViewportTop = std::make_unique<ViewportElementOrthographic<EViewportType::TopXZ>>( rtvFormat, dsvFormat, clearColor, mAntialiasingEnabled );
	mViewportFront = std::make_unique<ViewportElementOrthographic<EViewportType::FrontXY>>( rtvFormat, dsvFormat, clearColor, mAntialiasingEnabled );
	mViewportSide = std::make_unique<ViewportElementOrthographic<EViewportType::SideYZ>>( rtvFormat, dsvFormat, clearColor, mAntialiasingEnabled );
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
	if ( ImGui::MenuItem( "Autosize Viewports", "Ctrl+A" ) ) mShouldAutosize = true;
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
	ImVec2 viewSizePerspective, viewSizeTop, viewSizeFront, viewSizeSide;
	ImVec2 viewSize = ImGui::GetWindowSize();

	// Instantiate all windows and grab their positional data
	{
		if ( mShouldAutosize ) {
			mShouldAutosize = false;
			ImGui::SetNextWindowSize( { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 } );
		}

		ImGui::PushStyleVar( ImGuiStyleVar_WindowMinSize, { kMinViewportSize, kMinViewportSize } );

		ImGui::SetNextWindowSizeConstraints( { kMinViewportSize, kMinViewportSize }, { viewSize.x - kMinViewportSize, viewSize.y - kMinViewportSize } );
		if ( ImGui::BeginChild( "PerspectiveView", { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
			mViewportPerspective->UpdateViewportData();
			viewSizePerspective = mViewportPerspective->GetViewportData().mViewSize;
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if ( ImGui::BeginChild( "TopView", { ImGui::GetContentRegionAvail().x, viewSizePerspective.y }, ImGuiChildFlags_Borders, viewportFlags ) ) {
			mViewportTop->UpdateViewportData();
			viewSizeTop = mViewportTop->GetViewportData().mViewSize;
		}
		ImGui::EndChild();

		if ( ImGui::BeginChild( "FrontView", { viewSizePerspective.x, ImGui::GetContentRegionAvail().y }, ImGuiChildFlags_Borders, viewportFlags ) ) {
			mViewportFront->UpdateViewportData();
			viewSizeFront = mViewportFront->GetViewportData().mViewSize;
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
			mViewportSide->UpdateViewportData();
			viewSizeSide = mViewportSide->GetViewportData().mViewSize;
		}
		ImGui::EndChild();

		ImGui::PopStyleVar( 1 );
	}

	// Perform navigation and setup
	{
		ImGui::SetNextWindowSizeConstraints( viewSizePerspective, viewSizePerspective );
		if ( ImGui::BeginChild( "PerspectiveView", viewSizePerspective ) ) {
			mViewportPerspective->UpdateNavigation( inDeltaTime, inLevelInterface );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeTop, viewSizeTop );
		if ( ImGui::BeginChild( "TopView", viewSizeTop ) ) {
			mViewportTop->UpdateNavigation( inDeltaTime, inLevelInterface );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeFront, viewSizeFront );
		if ( ImGui::BeginChild( "FrontView", viewSizeFront ) ) {
			mViewportFront->UpdateNavigation( inDeltaTime, inLevelInterface );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeSide, viewSizeSide );
		if ( ImGui::BeginChild( "SideView", viewSizeSide ) ) {
			mViewportSide->UpdateNavigation( inDeltaTime, inLevelInterface );
		}
		ImGui::EndChild();

		// Clamp viewport
		const auto &gridContext = inLevelInterface->GetGridCtx();
		auto &orthographicContext = inLevelInterface->GetOrthographicCtx();
		orthographicContext.mCenter2D = Vector4D::sClamp( orthographicContext.mCenter2D, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

		// Update draw lists and selection box for tooling.
		// Technically we should update draw lists *after* tooling,
		// but this would be slower and due to the iteration costs.
		UpdateDrawListAndSelectionBox( inLevelInterface );
	}

	// Perform tooling updates
	{
		ImGui::SetNextWindowSizeConstraints( viewSizePerspective, viewSizePerspective );
		if ( ImGui::BeginChild( "PerspectiveView", viewSizePerspective ) ) {
			mViewportPerspective->UpdateTools( inDeltaTime, inLevelInterface, inTools );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeTop, viewSizeTop );
		if ( ImGui::BeginChild( "TopView", viewSizeTop ) ) {
			mViewportTop->UpdateTools( inDeltaTime, inLevelInterface, inTools );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeFront, viewSizeFront );
		if ( ImGui::BeginChild( "FrontView", viewSizeFront ) ) {
			mViewportFront->UpdateTools( inDeltaTime, inLevelInterface, inTools );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeSide, viewSizeSide );
		if ( ImGui::BeginChild( "SideView", viewSizeSide ) ) {
			mViewportSide->UpdateTools( inDeltaTime, inLevelInterface, inTools );
		}
		ImGui::EndChild();
	}

	// Perform gizmo draws
	{
		ImGui::SetNextWindowSizeConstraints( viewSizePerspective, viewSizePerspective );
		if ( ImGui::BeginChild( "PerspectiveView", viewSizePerspective ) ) {
			mViewportPerspective->DrawGizmos( inDeltaTime, inLevelInterface, inTools );
			DrawViewportOverlay( "Perspective" );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeTop, viewSizeTop );
		if ( ImGui::BeginChild( "TopView", viewSizeTop ) ) {
			mViewportTop->DrawGizmos( inDeltaTime, inLevelInterface, inTools );
			DrawViewportOverlay( "Top (X/Z)" );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeFront, viewSizeFront );
		if ( ImGui::BeginChild( "FrontView", viewSizeFront ) ) {
			mViewportFront->DrawGizmos( inDeltaTime, inLevelInterface, inTools );
			DrawViewportOverlay( "Front (X/Y)" );
		}
		ImGui::EndChild();

		ImGui::SetNextWindowSizeConstraints( viewSizeSide, viewSizeSide );
		if ( ImGui::BeginChild( "SideView", viewSizeSide ) ) {
			mViewportSide->DrawGizmos( inDeltaTime, inLevelInterface, inTools );
			DrawViewportOverlay( "Side (Y/Z)" );
		}
		ImGui::EndChild();
	}
}

void Cyclone::UI::ViewportManager::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	mViewportPerspective->Render( inDeviceContext, inLevelInterface, inTools );
	mViewportTop->Render( inDeviceContext, inLevelInterface, inTools );
	mViewportFront->Render( inDeviceContext, inLevelInterface, inTools );
	mViewportSide->Render( inDeviceContext, inLevelInterface, inTools );
}

void Cyclone::UI::ViewportManager::ToggleAntialiasing( bool inEnabled )
{
	mAntialiasingEnabled = inEnabled;

	mViewportPerspective->ToggleAntialiasing( inEnabled );
	mViewportTop->ToggleAntialiasing( inEnabled );
	mViewportFront->ToggleAntialiasing( inEnabled );
	mViewportSide->ToggleAntialiasing( inEnabled );
}

void Cyclone::UI::ViewportManager::UpdateDrawListAndSelectionBox( Cyclone::Core::LevelInterface *inLevelInterface )
{
	entt::registry &registry = inLevelInterface->GetRegistry();
	auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &transformContext = inLevelInterface->GetSelectionTransformCtx();
	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();
	const auto &gridContext = inLevelInterface->GetGridCtx();

	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();

	// Pre calculate full screen bounding boxes for the orthgraphic viewports
	Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> topBox{ orthographicContext.mCenter2D, mViewportTop->GetViewBoundingBoxExtent( gridContext.mWorldLimit, orthographicContext.mZoomScale2D ) };
	Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> frontBox{ orthographicContext.mCenter2D, mViewportFront->GetViewBoundingBoxExtent( gridContext.mWorldLimit, orthographicContext.mZoomScale2D ) };
	Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> sideBox{ orthographicContext.mCenter2D, mViewportSide->GetViewBoundingBoxExtent( gridContext.mWorldLimit, orthographicContext.mZoomScale2D ) };

	// Clear all visibility tags first
	registry.clear<entt::tag<"draw_perspective"_hs>, entt::tag<"draw_top"_hs>, entt::tag<"draw_front"_hs>, entt::tag<"draw_side"_hs>>();

	// Clear the transform selection bounding box
	transformContext.OnPreUpdate();

	auto view = registry.group<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox, entt::tag<"is_visible"_hs>>();
	for ( const entt::entity entity : view ) {
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
		const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity );

		bool entityInSelection = selectedEntities.contains( entity );

		// Add entity to selection bounding box
		if ( entityInSelection ) {
			transformContext.IncludeSelectedEntity( position, boundingBox );
		}

		// Create bounding boxes for the entity AABB and the position handle
		Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> entBox{ position.mValue + boundingBox.mValue.mCenter, boundingBox.mValue.mExtent };
		Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> posBox{ position.mValue, Cyclone::Math::Vector4D::sZero() };

		// If intersection append relevant draw tag
		if ( topBox.Intersects( entBox ) || topBox.Intersects( posBox ) ) registry.emplace<ViewportTypeTraits<EViewportType::TopXZ>::DrawTag>( entity );
		if ( frontBox.Intersects( entBox ) || frontBox.Intersects( posBox ) ) registry.emplace<ViewportTypeTraits<EViewportType::FrontXY>::DrawTag>( entity );
		if ( sideBox.Intersects( entBox ) || sideBox.Intersects( posBox ) ) registry.emplace<ViewportTypeTraits<EViewportType::SideYZ>::DrawTag>( entity );
	}
}
