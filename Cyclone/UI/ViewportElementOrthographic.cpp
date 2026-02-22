#include "pch.h"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"

// Cyclone UI Tools
#include "Cyclone/UI/Tool/SelectionTool.hpp"
#include "Cyclone/UI/Tool/SelectionTransformTool.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"
#include "Cyclone/Util/String.hpp"

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
void Cyclone::UI::ViewportElementOrthographic<T>::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const auto &gridContext = inLevelInterface->GetGridCtx();
	auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	ImVec2 viewSize = ImGui::GetWindowSize();
	ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->Flags |= ImDrawListFlags_AntiAliasedLines;

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::Image( GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	if( !ImGui::IsMouseDown( ImGuiMouseButton_Middle ) ) ImGui::SetNextItemAllowOverlap(); // Ensure middle mouse "wins" over selection
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	const bool isCanvasHovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenOverlappedByItem | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem );
	const bool isCanvasActive = ImGui::IsItemActive();
	const bool isLeftClickShort = ImGui::IsMouseReleased( 0 ) && io.MouseDownDurationPrev[0] < io.MouseDoubleClickTime;

	if ( isCanvasHovered || isCanvasActive ) ImGui::SetItemKeyOwner( ImGuiMod_Alt );

	ImVec2 viewportAbsMousePos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
	ImVec2 viewportRelMousePos( viewportAbsMousePos.x - viewSize.x / 2.0f, viewportAbsMousePos.y - viewSize.y / 2.0f );

	double worldMouseU = orthographicContext.mCenter2D.Get<AxisU>() - viewportRelMousePos.x * orthographicContext.mZoomScale2D;
	double worldMouseV = orthographicContext.mCenter2D.Get<AxisV>() - viewportRelMousePos.y * orthographicContext.mZoomScale2D;

	double worldSnapU = std::round( worldMouseU / gridContext.mGridSize ) * gridContext.mGridSize;
	double worldSnapV = std::round( worldMouseV / gridContext.mGridSize ) * gridContext.mGridSize;

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
		orthographicContext.mZoomScale2D
	);
	ImGui::PopStyleVar( 2 );

	if ( isCanvasHovered && isLeftClickShort ) {
		Tool::SelectionTool().OnClick<T>( inLevelInterface, worldMouseU, worldMouseV, 2.0f * kPositionHandleSize * orthographicContext.mZoomScale2D, gridContext.mWorldLimit );
	}

	// Middle click pan view
	if ( ( isCanvasActive ) && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) && !ImGui::IsMouseDragging( ImGuiMouseButton_Left, 0.0f ) ) {
		orthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisU>( io.MouseDelta.x * orthographicContext.mZoomScale2D );
		orthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisV>( io.MouseDelta.y * orthographicContext.mZoomScale2D );
	}

	// Zoom view
	if ( isCanvasHovered && io.MouseWheel && !ImGui::IsMouseDragging( ImGuiMouseButton_Left, 0.0f ) ) {
		int newZoomLevel = orthographicContext.mZoomLevel - ( ( io.MouseWheel > 0 ) - ( io.MouseWheel < 0 ) );
		double newZoomScale2D = orthographicContext.sZoomLevelToScale( newZoomLevel );

		double uPosNew = orthographicContext.mCenter2D.Get<AxisU>() - viewportRelMousePos.x * newZoomScale2D;
		double vPosNew = orthographicContext.mCenter2D.Get<AxisV>() - viewportRelMousePos.y * newZoomScale2D;

		orthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisU>( std::lerp( worldMouseU, worldSnapU, static_cast<double>( inDeltaTime * kAccelerateToSnap ) ) - uPosNew );
		orthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisV>( std::lerp( worldMouseV, worldSnapV, static_cast<double>( inDeltaTime * kAccelerateToSnap ) ) - vPosNew );

		orthographicContext.UpdateZoomLevel( newZoomLevel );
	}

	// Draw cursor
	if ( isCanvasHovered ) {
		ImVec2 gridPos;
		gridPos.x = static_cast<float>( ( orthographicContext.mCenter2D.Get<AxisU>() - worldSnapU ) / orthographicContext.mZoomScale2D + viewSize.x / 2.0f + viewOrigin.x );
		gridPos.y = static_cast<float>( ( orthographicContext.mCenter2D.Get<AxisV>() - worldSnapV ) / orthographicContext.mZoomScale2D + viewSize.y / 2.0f + viewOrigin.y );

		DrawCross( drawList, gridPos, 2.0f, IM_COL32( 255, 255, 255, 255 ) );
	}

	// Draw entites and get selection bounding box
	ImVec2 selectedBoxMin, selectedBoxMax;
	DrawEntities( inLevelInterface, drawList, viewOrigin, viewSize, selectedBoxMin, selectedBoxMax );

	// Perform selection transform
	Tool::SelectionTransformTool().OnUpdate<T>( inLevelInterface, drawList, viewOrigin, selectedBoxMin, selectedBoxMax );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{

	constexpr size_t AxisU = ViewportElementOrthographic::AxisU;
	constexpr size_t AxisV = ViewportElementOrthographic::AxisV;

	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>( gridContext.mWorldLimit ), GetProjMatrix( mWidth, mHeight, orthographicContext.mZoomScale2D, gridContext.mWorldLimit ) );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		double minU, maxU, minV, maxV;
		GetMinMaxUV( orthographicContext.mCenter2D, gridContext.mWorldLimit, orthographicContext.mZoomScale2D, minU, maxU, minV, maxV );

		double subgridStep = gridContext.mGridSize;
		double gridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;

		while ( subgridStep / orthographicContext.mZoomScale2D < kMinGridSize ) {
			//subgridStep *= 10;
			subgridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;
		}

		while ( gridStep / orthographicContext.mZoomScale2D < kMinGridSize * 5 ) {
			//gridStep *= 10;
			gridStep = std::pow( 10.0, std::ceil( std::log10( gridStep * 4 ) ) ) / 2;
		}

		if ( subgridStep / orthographicContext.mZoomScale2D > kMinGridSize ) {
			DrawLineLoop<AxisU, AxisV>( orthographicContext.mCenter2D, minU, maxU, minV, maxV, subgridStep, DirectX::ColorsLinear::DimGray );
			DrawLineLoop<AxisV, AxisU>( orthographicContext.mCenter2D, minV, maxV, minU, maxU, subgridStep, DirectX::ColorsLinear::DimGray );
		}

		DrawLineLoop<AxisU, AxisV>( orthographicContext.mCenter2D, minU, maxU, minV, maxV, gridStep, DirectX::Colors::DimGray );
		DrawLineLoop<AxisV, AxisU>( orthographicContext.mCenter2D, minV, maxV, minU, maxU, gridStep, DirectX::Colors::DimGray );

		DrawLineLoop<AxisU, AxisV>( orthographicContext.mCenter2D, minU, maxU, minV, maxV, 1000, DirectX::Colors::Gray );
		DrawLineLoop<AxisV, AxisU>( orthographicContext.mCenter2D, minV, maxV, minU, maxU, 1000, DirectX::Colors::Gray );

		DrawAxisLine<AxisU>( orthographicContext.mCenter2D, minU, maxU );
		DrawAxisLine<AxisV>( orthographicContext.mCenter2D, minV, maxV );
	}
	mWireframeGridBatch->End();


	// Switch to depth buffer
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthDefault(), 0 );

	mWireframeGridBatch->Begin();
	{
		const auto &selectionContext = inLevelInterface->GetSelectionCtx();
		const auto &entityContext = inLevelInterface->GetEntityCtx();

		const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();
		const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

		// Iterate over all entities
		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::EntityCategory, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox, Cyclone::Core::Component::Visible>();
		for ( const entt::entity entity : view ) {

			const auto &entityCategory = view.get<Cyclone::Core::Component::EntityCategory>( entity );
			if ( !*entityContext.GetEntityCategoryIsVisible( entityCategory ) ) continue;

			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			if ( !*entityContext.GetEntityTypeIsVisible( entityType ) ) continue;

			if ( !static_cast<bool>( view.get<Cyclone::Core::Component::Visible>( entity ) ) ) continue;

			const auto &position = view.get<Cyclone::Core::Component::Position>( entity ).mValue;
			const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity ).mValue;

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
				entityColorU32 = inLevelInterface->GetEntityCtx().GetEntityTypeColor( entityType );
			}

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

			Vector4D rebasedEntityPosition = ( position - orthographicContext.mCenter2D );
			Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			Cyclone::Util::RenderWireframeBoundingBox( mWireframeGridBatch.get(), rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}
	}
	mWireframeGridBatch->End();

	Resolve( inDeviceContext );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::DrawEntities( const Cyclone::Core::LevelInterface *inLevelInterface, ImDrawList *drawList, const ImVec2 &inViewOrigin, const ImVec2 &inViewSize, ImVec2 &outSelectedBoxMin, ImVec2 &outSelectedBoxMax ) const
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	// Split channels into 3 planes
	drawList->ChannelsSplit( 3 );
	drawList->ChannelsSetCurrent( 0 );

	// Get smaller font for debug text
	ImGuiIO &io = ImGui::GetIO();
	ImFont* narrowFont = io.Fonts->Fonts[1];
	float fontSize = ImGui::GetFontSize();

	outSelectedBoxMin = { inViewOrigin.x + inViewSize.x, inViewOrigin.y + inViewSize.y };
	outSelectedBoxMax = inViewOrigin;

	const double invZoom = 1.0 / orthographicContext.mZoomScale2D;
	const float offsetX = inViewSize.x / 2.0f + inViewOrigin.x;
	const float offsetY = inViewSize.y / 2.0f + inViewOrigin.y;

	ImVec2 maxViewCoord{ inViewOrigin.x + inViewSize.x, inViewOrigin.y + inViewSize.y };

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityContext = inLevelInterface->GetEntityCtx();

	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	// Iterate over all entities
	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::EntityCategory, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox, Cyclone::Core::Component::Visible>();
	for ( const entt::entity entity : view ) {

		const auto &entityCategory = view.get<Cyclone::Core::Component::EntityCategory>( entity );
		if ( !*entityContext.GetEntityCategoryIsVisible( entityCategory ) ) continue;

		const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
		if ( !*entityContext.GetEntityTypeIsVisible( entityType ) ) continue;

		if ( !static_cast<bool>( view.get<Cyclone::Core::Component::Visible>( entity ) ) ) continue;

		const auto &position = view.get<Cyclone::Core::Component::Position>( entity ).mValue;
		const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity ).mValue;


		auto entityColor = inLevelInterface->GetEntityCtx().GetEntityTypeColor( entityType );

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

		Vector4D rebasedEntityPosition = ( orthographicContext.mCenter2D - position );
		Vector4D rebasedBoundingBoxMin = rebasedEntityPosition - boundingBox.mCenter - boundingBox.mExtent;
		Vector4D rebasedBoundingBoxMax = rebasedEntityPosition - boundingBox.mCenter + boundingBox.mExtent;

		ImVec2 localBoxMin;
		localBoxMin.x = static_cast<float>( rebasedBoundingBoxMin.Get<AxisU>() * invZoom ) + offsetX;
		localBoxMin.y = static_cast<float>( rebasedBoundingBoxMin.Get<AxisV>() * invZoom ) + offsetY;

		ImVec2 localBoxMax;
		localBoxMax.x = static_cast<float>( rebasedBoundingBoxMax.Get<AxisU>() * invZoom ) + offsetX;
		localBoxMax.y = static_cast<float>( rebasedBoundingBoxMax.Get<AxisV>() * invZoom ) + offsetY;

		ImVec2 localPos;
		localPos.x = static_cast<float>( rebasedEntityPosition.Get<AxisU>() * invZoom ) + offsetX;
		localPos.y = static_cast<float>( rebasedEntityPosition.Get<AxisV>() * invZoom ) + offsetY;


		bool posInBounds = true;
		bool boxInBounds = true;

		if ( posInBounds && ( localPos.x < inViewOrigin.x || localPos.y < inViewOrigin.y ) ) posInBounds &= false;
		if ( posInBounds && ( localPos.x > maxViewCoord.x || localPos.y > maxViewCoord.y ) ) posInBounds &= false;

		if ( boxInBounds && ( localBoxMax.x < inViewOrigin.x || localBoxMax.y < inViewOrigin.y ) ) boxInBounds &= false;
		if ( boxInBounds && ( localBoxMin.x > maxViewCoord.x || localBoxMin.y > maxViewCoord.y ) ) boxInBounds &= false;

		bool inBounds = posInBounds || boxInBounds;

		// Only draw X if smaller than bounding box
		if ( kPositionHandleSize * 2 <= localBoxMax.x - localBoxMin.x && kPositionHandleSize * 2 <= localBoxMax.y - localBoxMin.y && inBounds ) {
			DrawCross( drawList, localPos, kPositionHandleSize, entityColor );
		}

		if ( kInformationVirtualSize * 2 <= localBoxMax.x - localBoxMin.x && kInformationVirtualSize * 2 <= localBoxMax.y - localBoxMin.y && inBounds ) {
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMin.y - ImGui::GetTextLineHeight() }, entityColor, inLevelInterface->GetEntityCtx().GetEntityTypeName( entityType ) );
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMax.y }, entityColor, Cyclone::Util::PrefixString( "id=", entity ) );
		}

		if ( entityInSelection ) {
			if ( inBounds ) {
				drawList->AddRect( localBoxMin, localBoxMax, entityColor, 0, 0, 2 );
			}

			outSelectedBoxMin.x = std::min( outSelectedBoxMin.x, localBoxMin.x );
			outSelectedBoxMin.y = std::min( outSelectedBoxMin.y, localBoxMin.y );

			outSelectedBoxMax.x = std::max( outSelectedBoxMax.x, localBoxMax.x );
			outSelectedBoxMax.y = std::max( outSelectedBoxMax.y, localBoxMax.y );
		}
	}

	drawList->ChannelsMerge();
}

template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::TopXZ>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::FrontXY>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::SideYZ>;
