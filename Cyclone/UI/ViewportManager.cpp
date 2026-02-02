#include "pch.h"

#include "Cyclone/UI/ViewportManager.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui_internal.h>

// DX Includes
#include <DirectXHelpers.h>

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
	mViewportPerspective = std::make_unique<ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::CornflowerBlue );
	mViewportTop = std::make_unique<ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportFront = std::make_unique<ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportSide = std::make_unique<ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );

	mCommonStates = std::make_unique<DirectX::CommonStates>( inDevice );

	mWireframeEffect = std::make_unique<DirectX::BasicEffect>( inDevice );
	mWireframeEffect->SetVertexColorEnabled( true );

	DX::ThrowIfFailed( DirectX::CreateInputLayoutFromEffect<DirectX::VertexPositionColor>( inDevice, mWireframeEffect.get(), mWireFrameInputLayout.ReleaseAndGetAddressOf() ) );

	ID3D11DeviceContext3 *deviceContext = nullptr;
	inDevice->GetImmediateContext3( &deviceContext );

	mWireframeBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>( deviceContext );
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime )
{
	ImGuiIO &io = ImGui::GetIO();

	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	ImVec2 perspectiveViewSize;

	ImGui::SetNextWindowSizeConstraints( { 64.0f, 64.0f }, { ImGui::GetContentRegionAvail().x - 64.0f, ImGui::GetContentRegionAvail().y - 64.0f } );
	if ( ImGui::BeginChild( "PerspectiveView", ImVec2( ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
		perspectiveViewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportPerspective->GetOrResizeSRV( static_cast<size_t>( perspectiveViewSize.x ), static_cast<size_t>( perspectiveViewSize.y ) ), perspectiveViewSize );

		DrawViewportOverlay( "Perspective" );

		ImGui::EndChild();
	}

	ImGui::SameLine();
	if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, perspectiveViewSize.y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

		ImGui::SetCursorPos( { 0, 0 } );
		ImGui::Image( mViewportTop->GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

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
				"World Pos: (x=%.1f, z=%.1f)\n"
				"Zoom Level: (%.3f)",
				io.MousePos.x, io.MousePos.y,
				viewportAbsPos.x, viewportAbsPos.y,
				viewportRelPos.x, viewportRelPos.y,
				xPos, zPos,
				mZoomScale2D
			);

			if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
				mCenterX2D += io.MouseDelta.x * mZoomScale2D;
				mCenterZ2D += io.MouseDelta.y * mZoomScale2D;
			}

			if ( isHovered ) {
				mZoomLevel -= io.MouseWheel;
				mZoomScale2D = std::pow( 10.0, static_cast<double>( mZoomLevel ) / 10.0 - 1.0 );
			}
		}
		ImGui::PopStyleVar( 2 );

		DrawViewportOverlay( "Top (X/Z)" );

		ImGui::EndChild();
	}

	if ( ImGui::BeginChild( "FrontView", ImVec2( perspectiveViewSize.x, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		ImVec2 viewSize = ImGui::GetWindowSize();
		ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

		ImGui::SetCursorPos( { 0, 0 } );
		ImGui::Image( mViewportFront->GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

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
				"World Pos: (x=%.1f, y=%.1f)\n"
				"Zoom Level: (%.3f)",
				io.MousePos.x, io.MousePos.y,
				viewportAbsPos.x, viewportAbsPos.y,
				viewportRelPos.x, viewportRelPos.y,
				xPos, yPos,
				mZoomScale2D
			);

			if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
				mCenterX2D += io.MouseDelta.x * mZoomScale2D;
				mCenterY2D += io.MouseDelta.y * mZoomScale2D;
			}

			if ( isHovered ) {
				mZoomLevel -= io.MouseWheel;
				mZoomScale2D = std::pow( 10.0, static_cast<double>( mZoomLevel ) / 10.0 - 1.0 );
			}
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
		ImGui::Image( mViewportSide->GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

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
				"World Pos: (z=%.1f, y=%.1f)\n"
				"Zoom Level: (%.3f)",
				io.MousePos.x, io.MousePos.y,
				viewportAbsPos.x, viewportAbsPos.y,
				viewportRelPos.x, viewportRelPos.y,
				zPos, yPos,
				mZoomScale2D
			);

			if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
				mCenterY2D += io.MouseDelta.y * mZoomScale2D;
				mCenterZ2D += io.MouseDelta.x * mZoomScale2D;
			}

			if ( isHovered ) {
				mZoomLevel -= io.MouseWheel;
				mZoomScale2D = std::pow( 10.0, static_cast<double>( mZoomLevel ) / 10.0 - 1.0 );
			}
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
	RenderWireFrame<EViewportType::TopXZ>( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderFront( ID3D11DeviceContext3 *inDeviceContext )
{
	RenderWireFrame<EViewportType::FrontXY>( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderSide( ID3D11DeviceContext3 *inDeviceContext )
{
	RenderWireFrame<EViewportType::SideYZ>( inDeviceContext );
}


template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportManager::RenderWireFrame( ID3D11DeviceContext3 *inDeviceContext )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	ViewportElement *viewport = GetViewport<T>();

	viewport->Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireFrameInputLayout.Get() );

	mWireframeEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>(), GetProjMatrix<T>() );
	mWireframeEffect->Apply( inDeviceContext );

	mWireframeBatch->Begin();

	double minU, maxU, minV, maxV;
	GetMinMaxUV<T>( minU, maxU, minV, maxV );

	double subgridStep = mSubGridSize;
	double gridStep = mGridSize;

	if ( subgridStep / mZoomScale2D > mMinGridSize ) {
		DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, subgridStep, DirectX::ColorsLinear::DimGray );
		DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, subgridStep, DirectX::ColorsLinear::DimGray );
	}

	DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, gridStep, DirectX::Colors::DimGray );
	DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, gridStep, DirectX::Colors::DimGray );

	DrawAxisLine<AxisU>( minU, maxU );
	DrawAxisLine<AxisV>( minV, maxV );

	mWireframeBatch->End();

	viewport->Resolve( inDeviceContext );
}