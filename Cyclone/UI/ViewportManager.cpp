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
	auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();

	orthographicContext.mCenter2D = Vector4D::sClamp( orthographicContext.mCenter2D, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityManager = inLevelInterface->GetEntityManager();
	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	ID3D11Device *device = inLevelInterface->GetDevice();
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
	device->GetImmediateContext( deviceContext.GetAddressOf() );
	ID3D11DeviceContext *pContext = deviceContext.Get();

	entt::registry &registry = inLevelInterface->GetRegistry();

	auto view1 = registry.view<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>( entt::exclude<Cyclone::Core::Component::RenderingBoundingBoxPerspective, Cyclone::Core::Component::RenderingBoundingBoxPerspective> );
	for ( const entt::entity entity : view1 ) {
		registry.emplace<Cyclone::Core::Component::RenderingBoundingBoxOrthographic>( entity, device );
		registry.emplace<Cyclone::Core::Component::RenderingBoundingBoxPerspective>( entity, device );
	}

	auto view = registry.group<>( entt::get<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox, Cyclone::Core::Component::RenderingBoundingBoxOrthographic, Cyclone::Core::Component::RenderingBoundingBoxPerspective> );
	for ( const entt::entity entity : view ) {
		bool entityInSelection = selectedEntities.contains( entity );
		bool entityIsSelected = selectedEntity == entity;

		const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity ).mValue;
		const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity ).mValue;

		uint32_t entityColorU32;
		if ( entityIsSelected ) {
			entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
		}
		else if ( entityInSelection ) {
			entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
		}
		else {
			entityColorU32 = entityManager.GetEntityTypeColor( entityType );
		}

		DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );
		
		{
			Vector4D rebasedEntityPosition = ( position - orthographicContext.mCenter2D );
			Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			auto &boxOrthographic = view.get<Cyclone::Core::Component::RenderingBoundingBoxOrthographic>( entity );
			boxOrthographic.Update( pContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}

		{
			Vector4D rebasedEntityPosition = ( position - perspectiveContext.mCenter3D );
			Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			auto &boxPerspective = view.get<Cyclone::Core::Component::RenderingBoundingBoxPerspective>( entity );
			boxPerspective.Update( pContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}

	}
}

void Cyclone::UI::ViewportManager::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	mViewportPerspective->Render( inDeviceContext, inLevelInterface );
	mViewportTop->Render( inDeviceContext, inLevelInterface );
	mViewportFront->Render( inDeviceContext, inLevelInterface );
	mViewportSide->Render( inDeviceContext, inLevelInterface );
}