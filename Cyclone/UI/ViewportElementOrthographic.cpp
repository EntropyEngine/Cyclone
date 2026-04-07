#include "pch.h"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"
#include "Cyclone/Util/String.hpp"

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
		float a, b, c, d;
		a = std::truncf( inOrigin.x - inWidth + 1 );
		b = std::truncf( inOrigin.y - inWidth + 1 );
		c = std::truncf( inOrigin.x + inWidth );
		d = std::truncf( inOrigin.y + inWidth );

		inDrawList->PathLineTo( { a, b } );
		inDrawList->PathLineTo( { c, d } );
		inDrawList->PathStroke( inColor );

		inDrawList->PathLineTo( { a, d } );
		inDrawList->PathLineTo( { c, b } );
		inDrawList->PathStroke( inColor );

		//inDrawList->AddLine( { inOrigin.x - inWidth, inOrigin.y - inWidth }, { inOrigin.x + inWidth, inOrigin.y + inWidth }, inColor );
		//inDrawList->AddLine( { inOrigin.x + inWidth, inOrigin.y - inWidth }, { inOrigin.x - inWidth, inOrigin.y + inWidth }, inColor );
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::UpdateNavigation( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const auto &gridContext = inLevelInterface->GetGridCtx();
	auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	ImVec2 &viewSize = mViewportData.mViewSize;
	ImVec2 &viewOrigin = mViewportData.mViewOrigin;
	ImDrawList *drawList = mViewportData.mDrawList;

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::Image( GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	if( !ImGui::IsMouseDown( ImGuiMouseButton_Middle ) ) ImGui::SetNextItemAllowOverlap(); // Ensure middle mouse "wins" over selection
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	mViewportData.mCanvasID = ImGui::GetItemID();

	const bool isCanvasHovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenOverlappedByItem | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem );
	const bool isCanvasActive = ImGui::IsItemActive();

	mViewportData.mIsActive = ImGui::GetCurrentContext()->ActiveIdWindow == ImGui::GetCurrentWindow() || ( ImGui::GetCurrentContext()->ActiveIdWindow == nullptr && isCanvasHovered );

	ImVec2 viewportAbsMousePos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
	ImVec2 viewportRelMousePos( viewportAbsMousePos.x - viewSize.x / 2.0f, viewportAbsMousePos.y - viewSize.y / 2.0f );

	double worldMouseU = orthographicContext.mCenter2D.Get<AxisU>() - viewportRelMousePos.x * orthographicContext.mZoomScale2D;
	double worldMouseV = orthographicContext.mCenter2D.Get<AxisV>() - viewportRelMousePos.y * orthographicContext.mZoomScale2D;

	mViewportData.mAbsoluteMouse = viewportAbsMousePos;

	mViewportData.mWorldMouseU = worldMouseU;
	mViewportData.mWorldMouseV = worldMouseV;

	double worldSnapU = std::round( worldMouseU / gridContext.mGridSize ) * gridContext.mGridSize;
	double worldSnapV = std::round( worldMouseV / gridContext.mGridSize ) * gridContext.mGridSize;

	//const char *uStr = std::array{ "x", "y", "z" }[AxisU];
	//const char *vStr = std::array{ "x", "y", "z" }[AxisV];

	//ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
	//ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
	//if ( isCanvasHovered ) ImGui::SetTooltip(
	//	"Mouse pos: (%.0f, %.0f)\n"
	//	"Viewport abs pos: (%.0f, %.0f)\n"
	//	"Viewport rel pos: (%.1f, %.1f)\n"
	//	"World Pos: (%s=%.2f, %s=%.2f)\n"
	//	"Snap Pos: (%s=%.2f, %s=%.2f)\n"
	//	"Zoom Level: (%.3f)",
	//	io.MousePos.x, io.MousePos.y,
	//	viewportAbsMousePos.x, viewportAbsMousePos.y,
	//	viewportRelMousePos.x, viewportRelMousePos.y,
	//	uStr, worldMouseU, vStr, worldMouseV,
	//	uStr, worldSnapU, vStr, worldSnapV,
	//	orthographicContext.mZoomScale2D
	//);
	//ImGui::PopStyleVar( 2 );

	// Middle click pan view
	if ( ( isCanvasActive ) && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) && !ImGui::IsMouseDragging( ImGuiMouseButton_Left, 0.0f ) && !ImGuizmo::IsUsingAny() ) {
		orthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisU>( io.MouseDelta.x * orthographicContext.mZoomScale2D );
		orthographicContext.mCenter2D += Vector4D::sZeroSetValueByIndex<AxisV>( io.MouseDelta.y * orthographicContext.mZoomScale2D );

		ImGui::SetKeyOwner( ImGuiKey_F24, mViewportData.mCanvasID, ImGuiInputFlags_LockThisFrame );
	}

	// Zoom view
	if ( mViewportData.mIsActive && io.MouseWheel && !ImGui::IsMouseDragging( ImGuiMouseButton_Left, 0.0f ) && !ImGuizmo::IsUsingAny() ) {
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

	// Update matrices
	mViewportData.mViewMatrix = GetViewMatrix<T>( gridContext.mWorldLimit );
	mViewportData.mProjMatrix = GetProjMatrix( mWidth, mHeight, orthographicContext.mZoomScale2D, gridContext.mWorldLimit );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::UpdateTools( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	for ( auto &tool : inTools ) {
		if ( tool->mIsSelected )
		tool->OnUpdate( T, inLevelInterface, mViewportData );
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::DrawGizmos( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	// Draw entites and get selection bounding box
	DrawEntities( inLevelInterface );

	for ( auto &tool : inTools ) {
		tool->OnDraw( T, inLevelInterface, mViewportData );
	}

	mViewportData.mDrawList->ChannelsMerge();
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	constexpr size_t AxisU = ViewportElementOrthographic::AxisU;
	constexpr size_t AxisV = ViewportElementOrthographic::AxisV;

	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	const DirectX::XMMATRIX viewMatrix = mViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = mViewportData.mProjMatrix;

	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mWireframeRasterStateMSAA.Get() : mWireframeRasterState.Get() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), viewMatrix, projMatrix );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		double minU, maxU, minV, maxV;
		GetMinMaxUV( orthographicContext.mCenter2D, gridContext.mWorldLimit, orthographicContext.mZoomScale2D, minU, maxU, minV, maxV );

		double subgridStep = gridContext.mGridSize;
		double gridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;

		while ( subgridStep / orthographicContext.mZoomScale2D < Cyclone::Core::Editor::GridContext::kMinGridSize ) {
			//subgridStep *= 10;
			subgridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;
		}

		while ( gridStep / orthographicContext.mZoomScale2D < Cyclone::Core::Editor::GridContext::kMinGridSize * 5 ) {
			//gridStep *= 10;
			gridStep = std::pow( 10.0, std::ceil( std::log10( gridStep * 4 ) ) ) / 2;
		}

		if ( subgridStep / orthographicContext.mZoomScale2D > Cyclone::Core::Editor::GridContext::kMinGridSize ) {
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

	ViewportElement::Render<T>( inDeviceContext, inLevelInterface, inTools );

	Resolve( inDeviceContext );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::DrawEntities( Cyclone::Core::LevelInterface *inLevelInterface ) const
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	ImDrawList *drawList = mViewportData.mDrawList;
	ImVec2 inViewSize = mViewportData.mViewSize;
	ImVec2 inViewOrigin = mViewportData.mViewOrigin;

	// Get smaller font for debug text
	ImGuiIO &io = ImGui::GetIO();
	ImFont* narrowFont = io.Fonts->Fonts[1];
	float fontSize = ImGui::GetFontSize();

	const double invZoom = 1.0 / orthographicContext.mZoomScale2D;
	const float offsetX = inViewSize.x / 2.0f + inViewOrigin.x;
	const float offsetY = inViewSize.y / 2.0f + inViewOrigin.y;

	ImVec2 maxViewCoord{ inViewOrigin.x + inViewSize.x, inViewOrigin.y + inViewSize.y };

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityManager = inLevelInterface->GetEntityManager();

	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	// Iterate over all entities
	entt::registry &registry = inLevelInterface->GetRegistry();
	auto view = registry.view<EntityType, Position, BoundingBox, ViewportTypeTraits<T>::DrawTag>();
	//view.use<BoundingBox>();
	for ( const entt::entity entity : view ) {
		const auto &entityType = view.get<EntityType>( entity );
		const auto &position = view.get<Position>( entity ).mValue;
		const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

		uint32_t entityColor;

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
			entityColor = entityManager.GetEntityTypeColor( entityType );
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

		// Only draw X if smaller than bounding box
		if ( mViewportData.mIsActive && Cyclone::Core::Editor::GridContext::kPositionHandleSize * 2 <= localBoxMax.x - localBoxMin.x && Cyclone::Core::Editor::GridContext::kPositionHandleSize * 2 <= localBoxMax.y - localBoxMin.y ) {
			DrawCross( drawList, localPos, Cyclone::Core::Editor::GridContext::kPositionHandleSize, entityColor );
		}

		if ( entityInSelection && Cyclone::Core::Editor::GridContext::kInformationVirtualSize * 2 <= localBoxMax.x - localBoxMin.x && Cyclone::Core::Editor::GridContext::kInformationVirtualSize * 2 <= localBoxMax.y - localBoxMin.y ) {
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMin.y - ImGui::GetTextLineHeight() }, entityColor, entityManager.GetEntityTypeName( entityType ) );
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMax.y }, entityColor, Cyclone::Util::PrefixString( "id=", entity ) );
		}

		if ( entityInSelection ) {
			//drawList->AddRect( localBoxMin, localBoxMax, entityColor, 0, 0, 2 );
		}

		//if ( inBounds ) {
		//	registry.emplace<ViewportTypeTraits<T>::DrawTag>( entity );
		//}
	}
}

template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::TopXZ>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::FrontXY>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::SideYZ>;
