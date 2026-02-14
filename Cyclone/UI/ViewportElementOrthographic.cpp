#include "pch.h"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"

// Cyclone UI Tools
#include "Cyclone/UI/Tool/SelectionTool.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"

// STL Includes
#include <format>

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

using Cyclone::Math::Vector4D;

namespace
{
	template<Cyclone::UI::EViewportType T>
	DirectX::XMMATRIX XM_CALLCONV GetViewMatrix( double inWorldLimit );

	template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<Cyclone::UI::EViewportType::TopXZ>( double inWorldLimit )
	{
		return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( 0.0f, static_cast<float>( 2 * inWorldLimit ), 0.0f, 0.0f ), -DirectX::g_XMIdentityR1, DirectX::g_XMIdentityR2 );
	}

	template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<Cyclone::UI::EViewportType::FrontXY>( double inWorldLimit )
	{
		return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( 0.0f, 0.0f, static_cast<float>( -2 * inWorldLimit ), 0.0f ), DirectX::g_XMIdentityR2, DirectX::g_XMIdentityR1 );
	}

	template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<Cyclone::UI::EViewportType::SideYZ>( double inWorldLimit )
	{
		return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( static_cast<float>( 2 * inWorldLimit ), 0.0f, 0.0f, 0.0f ), -DirectX::g_XMIdentityR0, DirectX::g_XMIdentityR1 );
	}

	
	DirectX::XMMATRIX XM_CALLCONV GetProjMatrix( size_t inWidth, size_t inHeight, double inZoomScale2D, double inWorldLimit )
	{
		return DirectX::XMMatrixOrthographicRH( static_cast<float>( inWidth * inZoomScale2D ), static_cast<float>( inHeight * inZoomScale2D ), 1.0f, static_cast<float>( 4 * inWorldLimit ) );
	}

	void DrawCross( ImDrawList *inDrawList, const ImVec2 &inOrigin, float inWidth, ImU32 inColor )
	{
		inDrawList->AddLine( { inOrigin.x - inWidth, inOrigin.y - inWidth }, { inOrigin.x + inWidth, inOrigin.y + inWidth }, inColor );
		inDrawList->AddLine( { inOrigin.x + inWidth, inOrigin.y - inWidth }, { inOrigin.x - inWidth, inOrigin.y + inWidth }, inColor );
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, ViewportGridContext& inGridContext, ViewportOrthographicContext& inOrthographicContext )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	ImVec2 viewSize = ImGui::GetWindowSize();
	ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->Flags |= ImDrawListFlags_AntiAliasedLines;

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::Image( GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::SetNextItemAllowOverlap();
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	const bool isCanvasHovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenOverlappedByItem );
	const bool isCanvasActive = ImGui::IsItemActive();
	const bool isCanvasClicked = io.MouseClicked[0];

	if ( isCanvasHovered || isCanvasActive ) ImGui::SetItemKeyOwner( ImGuiMod_Alt );

	ImVec2 viewportAbsMousePos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
	ImVec2 viewportRelMousePos( viewportAbsMousePos.x - viewSize.x / 2.0f, viewportAbsMousePos.y - viewSize.y / 2.0f );

	double worldMouseU = inOrthographicContext.mCenter2D.Get<AxisU>() - viewportRelMousePos.x * inOrthographicContext.mZoomScale2D;
	double worldMouseV = inOrthographicContext.mCenter2D.Get<AxisV>() - viewportRelMousePos.y * inOrthographicContext.mZoomScale2D;

	double worldSnapU = std::round( worldMouseU / inGridContext.mGridSize ) * inGridContext.mGridSize;
	double worldSnapV = std::round( worldMouseV / inGridContext.mGridSize ) * inGridContext.mGridSize;

	const char *uStr = std::array{ "x", "y", "z" }[AxisU];
	const char *vStr = std::array{ "x", "y", "z" }[AxisV];

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
	if ( isCanvasHovered ) ImGui::SetTooltip(
		"Mouse pos: (%.0f, %.0f)\n"
		"Viewport abs pos: (%.0f, %.0f)\n"
		"Viewport rel pos: (%.1f, %.1f)\n"
		"World Pos: (%s=%.2f, %s=%.2f)\n"
		"Snap Pos: (%s=%.2f, %s=%.2f)\n"
		"Zoom Level: (%.3f)",
		io.MousePos.x, io.MousePos.y,
		viewportAbsMousePos.x, viewportAbsMousePos.y,
		viewportRelMousePos.x, viewportRelMousePos.y,
		uStr, worldMouseU, vStr, worldMouseV,
		uStr, worldSnapU, vStr, worldSnapV,
		inOrthographicContext.mZoomScale2D
	);
	ImGui::PopStyleVar( 2 );

	if ( ( isCanvasActive || ( isCanvasHovered && ImGui::IsKeyDown( ImGuiMod_Ctrl ) ) ) && isCanvasClicked ) {
		Tool::SelectionTool().OnClick<T>( inLevelInterface, worldMouseU, worldMouseV, 2.0f * kPositionHandleSize * inOrthographicContext.mZoomScale2D, inGridContext.mWorldLimit );
	}

	// Middle click pan view
	if ( ( isCanvasHovered || isCanvasActive ) && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
		inOrthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisU>( io.MouseDelta.x * inOrthographicContext.mZoomScale2D );
		inOrthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisV>( io.MouseDelta.y * inOrthographicContext.mZoomScale2D );
	}

	// Zoom view
	if ( isCanvasHovered && io.MouseWheel ) {
		int newZoomLevel = inOrthographicContext.mZoomLevel - ( ( io.MouseWheel > 0 ) - ( io.MouseWheel < 0 ) );
		double newZoomScale2D = inOrthographicContext.sZoomLevelToScale( newZoomLevel );

		double uPosNew = inOrthographicContext.mCenter2D.Get<AxisU>() - viewportRelMousePos.x * newZoomScale2D;
		double vPosNew = inOrthographicContext.mCenter2D.Get<AxisV>() - viewportRelMousePos.y * newZoomScale2D;

		inOrthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisU>( std::lerp( worldMouseU, worldSnapU, static_cast<double>( inDeltaTime * kAccelerateToSnap ) ) - uPosNew );
		inOrthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisV>( std::lerp( worldMouseV, worldSnapV, static_cast<double>( inDeltaTime * kAccelerateToSnap ) ) - vPosNew );

		inOrthographicContext.UpdateZoomLevel( newZoomLevel );
	}

	// Draw cursor
	if ( isCanvasHovered ) {
		ImVec2 gridPos;
		gridPos.x = static_cast<float>( ( inOrthographicContext.mCenter2D.Get<AxisU>() - worldSnapU ) / inOrthographicContext.mZoomScale2D + viewSize.x / 2.0f + viewOrigin.x );
		gridPos.y = static_cast<float>( ( inOrthographicContext.mCenter2D.Get<AxisV>() - worldSnapV ) / inOrthographicContext.mZoomScale2D + viewSize.y / 2.0f + viewOrigin.y );

		DrawCross( drawList, gridPos, 2.0f, IM_COL32( 255, 255, 255, 255 ) );
	}

	// Draw entites and get selection bounding box
	ImVec2 selectedBoxMin, selectedBoxMax;
	DrawEntities( inLevelInterface, inOrthographicContext, drawList, viewOrigin, viewSize, selectedBoxMin, selectedBoxMax );

	// Perform drag and selection
	TransformSelection( inLevelInterface, inGridContext, inOrthographicContext, drawList, viewOrigin, selectedBoxMin, selectedBoxMax );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext )
{

	constexpr size_t AxisU = ViewportElementOrthographic::AxisU;
	constexpr size_t AxisV = ViewportElementOrthographic::AxisV;

	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>( inGridContext.mWorldLimit ), GetProjMatrix( mWidth, mHeight, inOrthographicContext.mZoomScale2D, inGridContext.mWorldLimit ) );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		double minU, maxU, minV, maxV;
		GetMinMaxUV( inOrthographicContext.mCenter2D, inGridContext.mWorldLimit, inOrthographicContext.mZoomScale2D, minU, maxU, minV, maxV );

		double subgridStep = inGridContext.mGridSize;
		double gridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;

		while ( subgridStep / inOrthographicContext.mZoomScale2D < kMinGridSize ) {
			//subgridStep *= 10;
			subgridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;
		}

		while ( gridStep / inOrthographicContext.mZoomScale2D < kMinGridSize * 5 ) {
			//gridStep *= 10;
			gridStep = std::pow( 10.0, std::ceil( std::log10( gridStep * 4 ) ) ) / 2;
		}

		if ( subgridStep / inOrthographicContext.mZoomScale2D > kMinGridSize ) {
			DrawLineLoop<AxisU, AxisV>( inOrthographicContext.mCenter2D, minU, maxU, minV, maxV, subgridStep, DirectX::ColorsLinear::DimGray );
			DrawLineLoop<AxisV, AxisU>( inOrthographicContext.mCenter2D, minV, maxV, minU, maxU, subgridStep, DirectX::ColorsLinear::DimGray );
		}

		DrawLineLoop<AxisU, AxisV>( inOrthographicContext.mCenter2D, minU, maxU, minV, maxV, gridStep, DirectX::Colors::DimGray );
		DrawLineLoop<AxisV, AxisU>( inOrthographicContext.mCenter2D, minV, maxV, minU, maxU, gridStep, DirectX::Colors::DimGray );

		DrawLineLoop<AxisU, AxisV>( inOrthographicContext.mCenter2D, minU, maxU, minV, maxV, 1000, DirectX::Colors::Gray );
		DrawLineLoop<AxisV, AxisU>( inOrthographicContext.mCenter2D, minV, maxV, minU, maxU, 1000, DirectX::Colors::Gray );

		DrawAxisLine<AxisU>( inOrthographicContext.mCenter2D, minU, maxU );
		DrawAxisLine<AxisV>( inOrthographicContext.mCenter2D, minV, maxV );
	}
	mWireframeGridBatch->End();


	// Switch to depth buffer
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthDefault(), 0 );

	mWireframeGridBatch->Begin();
	{
		const std::set<entt::entity> &selectedEntities = inLevelInterface->GetSelectedEntities();
		const entt::entity selectedEntity = inLevelInterface->GetSelectedEntity();

		// Iterate over all entities
		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>();
		for ( const entt::entity entity : view ) {

			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
			const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity );

			bool entityInSelection = selectedEntities.contains( entity );
			bool entityIsSelected = selectedEntity == entity;

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

			Vector4D rebasedEntityPosition = ( position - inOrthographicContext.mCenter2D );
			Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			Cyclone::Util::RenderWireframeBoundingBox( mWireframeGridBatch.get(), rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}
	}
	mWireframeGridBatch->End();

	Resolve( inDeviceContext );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::DrawEntities( const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportOrthographicContext &inOrthographicContext, ImDrawList *drawList, const ImVec2 &inViewOrigin, const ImVec2 &inViewSize, ImVec2 &outSelectedBoxMin, ImVec2 &outSelectedBoxMax ) const
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	// Split channels into 3 planes
	drawList->ChannelsSplit( 3 );
	drawList->ChannelsSetCurrent( 0 );

	// Get smaller font for debug text
	ImGuiIO &io = ImGui::GetIO();
	ImFont* narrowFont = io.Fonts->Fonts[1];
	float fontSize = ImGui::GetFontSize();

	outSelectedBoxMin = { inViewOrigin.x + inViewSize.x, inViewOrigin.y + inViewSize.y };
	outSelectedBoxMax = inViewOrigin;

	const double invZoom = 1.0 / inOrthographicContext.mZoomScale2D;
	const float offsetX = inViewSize.x / 2.0f + inViewOrigin.x;
	const float offsetY = inViewSize.y / 2.0f + inViewOrigin.y;

	const std::set<entt::entity> &selectedEntities = inLevelInterface->GetSelectedEntities();
	const entt::entity selectedEntity = inLevelInterface->GetSelectedEntity();

	// Iterate over all entities
	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>();
	for ( const entt::entity entity : view ) {

		const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
		const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity );

		auto entityColor = entt::resolve( static_cast<entt::id_type>( entityType ) ).data( "debug_color"_hs ).get( {} ).cast<uint32_t>();

		bool entityInSelection = selectedEntities.contains( entity );
		bool entityIsSelected = selectedEntity == entity;

		if ( entityIsSelected ) {
			entityColor = IM_COL32( 255, 255, 0, 255 );
			drawList->ChannelsSetCurrent( 2 );
		}
		else if ( entityInSelection ) {
			entityColor = IM_COL32( 255, 128, 0, 255 );
			drawList->ChannelsSetCurrent( 1 );
		}
		else {
			drawList->ChannelsSetCurrent( 0 );
		}

		Vector4D rebasedEntityPosition = ( inOrthographicContext.mCenter2D - position );
		Vector4D rebasedBoundingBoxMin = rebasedEntityPosition - boundingBox.mCenter - boundingBox.mExtent;
		Vector4D rebasedBoundingBoxMax = rebasedEntityPosition - boundingBox.mCenter + boundingBox.mExtent;

		ImVec2 localBoxMin;
		localBoxMin.x = static_cast<float>( rebasedBoundingBoxMin.Get<AxisU>() * invZoom + offsetX );
		localBoxMin.y = static_cast<float>( rebasedBoundingBoxMin.Get<AxisV>() * invZoom + offsetY );

		ImVec2 localBoxMax;
		localBoxMax.x = static_cast<float>( rebasedBoundingBoxMax.Get<AxisU>() * invZoom + offsetX );
		localBoxMax.y = static_cast<float>( rebasedBoundingBoxMax.Get<AxisV>() * invZoom + offsetY );

		ImVec2 localPos;
		localPos.x = static_cast<float>( rebasedEntityPosition.Get<AxisU>() * invZoom + offsetX );
		localPos.y = static_cast<float>( rebasedEntityPosition.Get<AxisV>() * invZoom + offsetY );

		// Only draw X if smaller than bounding box
		if ( kPositionHandleSize * 2 <= localBoxMax.x - localBoxMin.x && kPositionHandleSize * 2 <= localBoxMax.y - localBoxMin.y ) {
			DrawCross( drawList, localPos, kPositionHandleSize, entityColor );
		}

		if ( kInformationVirtualSize * 2 <= localBoxMax.x - localBoxMin.x && kInformationVirtualSize * 2 <= localBoxMax.y - localBoxMin.y ) {
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMin.y - ImGui::GetTextLineHeight() }, entityColor, Cyclone::Core::Entity::EntityTypeRegistry::GetEntityTypeName( entityType ) );
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMax.y }, entityColor, std::format( "id={}", static_cast<size_t>( entity ) ).c_str() );
		}

		if ( entityInSelection ) {
			drawList->AddRect( localBoxMin, localBoxMax, entityColor, 0, 0, 2 );

			outSelectedBoxMin.x = std::min( outSelectedBoxMin.x, localBoxMin.x );
			outSelectedBoxMin.y = std::min( outSelectedBoxMin.y, localBoxMin.y );

			outSelectedBoxMax.x = std::max( outSelectedBoxMax.x, localBoxMax.x );
			outSelectedBoxMax.y = std::max( outSelectedBoxMax.y, localBoxMax.y );
		}
	}

	drawList->ChannelsMerge();
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::TransformSelection( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext, ImDrawList *drawList, const ImVec2 &inViewOrigin, const ImVec2 &inSelectedBoxMin, const ImVec2 &inSelectedBoxMax )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const std::set<entt::entity> &selectedEntities = inLevelInterface->GetSelectedEntities();
	const entt::entity selectedEntity = inLevelInterface->GetSelectedEntity();

	if ( !selectedEntities.empty() ) {

		drawList->AddRect( inSelectedBoxMin, inSelectedBoxMax, IM_COL32( 255, 0, 0, 255 ), 0, 0, 2 );

		for ( float x = inSelectedBoxMin.x; x < inSelectedBoxMax.x - 8; x += 16 ) {
			drawList->AddLine( { x, inSelectedBoxMin.y }, { x + 8, inSelectedBoxMin.y }, IM_COL32( 255, 255, 0, 255 ), 2 );
			drawList->AddLine( { x - 1, inSelectedBoxMax.y - 1 }, { x + 7, inSelectedBoxMax.y - 1 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}

		for ( float y = inSelectedBoxMin.y; y < inSelectedBoxMax.y - 8; y += 16 ) {
			drawList->AddLine( { inSelectedBoxMin.x, y }, { inSelectedBoxMin.x, y + 8 }, IM_COL32( 255, 255, 0, 255 ), 2 );
			drawList->AddLine( { inSelectedBoxMax.x - 1, y - 1 }, { inSelectedBoxMax.x - 1, y + 7 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}
		/*
		ImGui::SetCursorPos( { inSelectedBoxMin.x - inViewOrigin.x, inSelectedBoxMin.y - inViewOrigin.y } );
		ImGui::InvisibleButton( "Selection", { inSelectedBoxMax.x - inSelectedBoxMin.x, inSelectedBoxMax.y - inSelectedBoxMin.y }, ImGuiButtonFlags_MouseButtonLeft );
		const bool isSelectionHovered = ImGui::IsItemHovered();
		const bool isSelectionActive = ImGui::IsItemActive();

		entt::registry &registry = inLevelInterface->GetRegistry();
		if ( isSelectionActive && ImGui::IsMouseDragging( ImGuiMouseButton_Left ) ) {

			ImVec2 selectionMouseDrag = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );

			auto &&positionDeltaStorage = registry.storage<Cyclone::Core::Component::Position>( "delta"_hs );

			Cyclone::Core::Component::Position currentPosition = registry.get<Cyclone::Core::Component::Position>( selectedEntity );

			if ( !positionDeltaStorage.contains( selectedEntity ) ) {
				positionDeltaStorage.emplace( selectedEntity, registry.get<Cyclone::Core::Component::Position>( selectedEntity ) );
			}

			Cyclone::Core::Component::Position startPosition = positionDeltaStorage.get( selectedEntity );
			Cyclone::Core::Component::Position positionDelta{ startPosition - currentPosition };

			if ( inGridContext.mSnapType != ViewportGridContext::ESnapType::None ) {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( std::round( -selectionMouseDrag.x * inOrthographicContext.mZoomScale2D / inGridContext.mGridSize ) * inGridContext.mGridSize );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( std::round( -selectionMouseDrag.y * inOrthographicContext.mZoomScale2D / inGridContext.mGridSize ) * inGridContext.mGridSize );

				if ( inGridContext.mSnapType == ViewportGridContext::ESnapType::ToGrid ) {
					positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( std::round( startPosition.Get<AxisU>() / inGridContext.mGridSize ) * inGridContext.mGridSize - startPosition.Get<AxisU>() );
					positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( std::round( startPosition.Get<AxisV>() / inGridContext.mGridSize ) * inGridContext.mGridSize - startPosition.Get<AxisV>() );
				}
			}
			else {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( -selectionMouseDrag.x * inOrthographicContext.mZoomScale2D );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( -selectionMouseDrag.y * inOrthographicContext.mZoomScale2D );
			}

			for ( const entt::entity entity : selectedEntities ) {
				registry.get<Cyclone::Core::Component::Position>( entity ) += positionDelta;
			}
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
			registry.storage<Cyclone::Core::Component::Position>( "delta"_hs ).clear();
		}
		*/
	}
}

template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::TopXZ>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::FrontXY>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::SideYZ>;
