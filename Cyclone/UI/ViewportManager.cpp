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
	mViewportPerspective = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::CornflowerBlue );
	mViewportTop = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportFront = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportSide = std::make_unique<Cyclone::UI::ViewportElement>( inDevice, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );

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
	mViewportTop->Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireFrameInputLayout.Get() );

	mWireframeEffect->SetMatrices(
		DirectX::XMMatrixIdentity(),
		DirectX::XMMatrixLookAtRH(
			DirectX::XMVectorSet( static_cast<float>( mCenterX2D ), 10000.0f, static_cast<float>( mCenterZ2D ), 0.0f ),
			DirectX::XMVectorSet( static_cast<float>( mCenterX2D ), -10000.0f, static_cast<float>( mCenterZ2D ), 0.0f ),
			DirectX::XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f )
		),
		DirectX::XMMatrixOrthographicRH( mViewportTop->GetWidth() * mZoomScale2D, mViewportTop->GetHeight() * mZoomScale2D, 1.0f, 20000.0f )
	);
	mWireframeEffect->Apply( inDeviceContext );

	mWireframeBatch->Begin();

	float minX = static_cast<float>( mViewportTop->GetWidth() * -mZoomScale2D / 2 + mCenterX2D );
	float maxX = static_cast<float>( mViewportTop->GetWidth() * mZoomScale2D / 2 + mCenterX2D );

	float minZ = static_cast<float>( mViewportTop->GetHeight() * -mZoomScale2D / 2 + mCenterZ2D );
	float maxZ = static_cast<float>( mViewportTop->GetHeight() * mZoomScale2D / 2 + mCenterZ2D );

	for ( float xLine = std::round( minX / mSubGridSize ) * mSubGridSize; xLine <= maxX; xLine += mSubGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( xLine, 0.0f, minZ, 0.0f ), DirectX::ColorsLinear::DimGray },
			{ DirectX::XMVectorSet( xLine, 0.0f, maxZ, 0.0f ), DirectX::ColorsLinear::DimGray }
		);
	}

	for ( float zLine = std::round( minZ / mSubGridSize ) * mSubGridSize; zLine <= maxZ; zLine += mSubGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( minX, 0.0f, zLine, 0.0f ), DirectX::ColorsLinear::DimGray },
			{ DirectX::XMVectorSet( maxX, 0.0f, zLine, 0.0f ), DirectX::ColorsLinear::DimGray }
		);
	}

	for ( float xLine = std::round( minX / mGridSize ) * mGridSize; xLine <= maxX; xLine += mGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( xLine, 0.0f, minZ, 0.0f ), DirectX::Colors::DimGray },
			{ DirectX::XMVectorSet( xLine, 0.0f, maxZ, 0.0f ), DirectX::Colors::DimGray }
		);
	}

	for ( float zLine = std::round( minZ / mGridSize ) * mGridSize; zLine <= maxZ; zLine += mGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( minX, 0.0f, zLine, 0.0f ), DirectX::Colors::DimGray },
			{ DirectX::XMVectorSet( maxX, 0.0f, zLine, 0.0f ), DirectX::Colors::DimGray }
		);
	}

	mWireframeBatch->DrawLine(
		{ DirectX::XMVectorSet( minX, 0.0f, 0.0f, 0.0f ), DirectX::Colors::DarkRed },
		{ DirectX::XMVectorSet( maxX, 0.0f, 0.0f, 0.0f ), DirectX::Colors::DarkRed }
	);

	mWireframeBatch->DrawLine(
		{ DirectX::XMVectorSet( 0.0f, 0.0f, minZ, 0.0f ), DirectX::Colors::DarkBlue },
		{ DirectX::XMVectorSet( 0.0f, 0.0f, maxZ, 0.0f ), DirectX::Colors::DarkBlue }
	);

	mWireframeBatch->End();

	mViewportTop->Resolve( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderFront( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportFront->Clear( inDeviceContext );
	
	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireFrameInputLayout.Get() );

	mWireframeEffect->SetMatrices(
		DirectX::XMMatrixIdentity(),
		DirectX::XMMatrixLookAtRH(
			DirectX::XMVectorSet( static_cast<float>( -mCenterX2D ), static_cast<float>( mCenterY2D ), 10000.0f, 0.0f ),
			DirectX::XMVectorSet( static_cast<float>( -mCenterX2D ), static_cast<float>( mCenterY2D ), -10000.0f, 0.0f ),
			DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f )
		),
		DirectX::XMMatrixOrthographicRH( mViewportFront->GetWidth() * mZoomScale2D, mViewportFront->GetHeight() * mZoomScale2D, 1.0f, 20000.0f )
	);
	mWireframeEffect->Apply( inDeviceContext );

	float minX = static_cast<float>( mViewportFront->GetWidth() * -mZoomScale2D / 2 - mCenterX2D );
	float maxX = static_cast<float>( mViewportFront->GetWidth() * mZoomScale2D / 2 - mCenterX2D );

	float minY = static_cast<float>( mViewportFront->GetHeight() * -mZoomScale2D / 2 + mCenterY2D );
	float maxY = static_cast<float>( mViewportFront->GetHeight() * mZoomScale2D / 2 + mCenterY2D );

	mWireframeBatch->Begin();

	for ( float xLine = std::round( minX / mSubGridSize ) * mSubGridSize; xLine <= maxX; xLine += mSubGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( xLine, minY, 0.0f, 0.0f ), DirectX::ColorsLinear::DimGray },
			{ DirectX::XMVectorSet( xLine, maxY, 0.0f, 0.0f ), DirectX::ColorsLinear::DimGray }
		);
	}

	for ( float yLine = std::round( minY / mSubGridSize ) * mSubGridSize; yLine <= maxY; yLine += mSubGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( minX, yLine, 0.0f, 0.0f ), DirectX::ColorsLinear::DimGray },
			{ DirectX::XMVectorSet( maxX, yLine, 0.0f, 0.0f ), DirectX::ColorsLinear::DimGray }
		);
	}

	for ( float xLine = std::round( minX / mGridSize ) * mGridSize; xLine <= maxX; xLine += mGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( xLine, minY, 0.0f, 0.0f ), DirectX::Colors::DimGray },
			{ DirectX::XMVectorSet( xLine, maxY, 0.0f, 0.0f ), DirectX::Colors::DimGray }
		);
	}

	for ( float yLine = std::round( minY / mGridSize ) * mGridSize; yLine <= maxY; yLine += mGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( minX, yLine, 0.0f, 0.0f ), DirectX::Colors::DimGray },
			{ DirectX::XMVectorSet( maxX, yLine, 0.0f, 0.0f ), DirectX::Colors::DimGray }
		);
	}

	mWireframeBatch->DrawLine(
		{ DirectX::XMVectorSet( minX, 0.0f, 0.0f, 0.0f ), DirectX::Colors::DarkRed },
		{ DirectX::XMVectorSet( maxX, 0.0f, 0.0f, 0.0f ), DirectX::Colors::DarkRed }
	);

	mWireframeBatch->DrawLine(
		{ DirectX::XMVectorSet( 0.0f, minY, 0.0f, 0.0f ), DirectX::Colors::Green },
		{ DirectX::XMVectorSet( 0.0f, maxY, 0.0f, 0.0f ), DirectX::Colors::Green }
	);
	mWireframeBatch->End();

	mViewportFront->Resolve( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderSide( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportSide->Clear( inDeviceContext );
	
	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireFrameInputLayout.Get() );

	mWireframeEffect->SetMatrices(
		DirectX::XMMatrixIdentity(),
		DirectX::XMMatrixLookAtRH(
			DirectX::XMVectorSet( 10000.0f, static_cast<float>( mCenterY2D ), static_cast<float>( mCenterZ2D ), 0.0f ),
			DirectX::XMVectorSet( -10000.0f, static_cast<float>( mCenterY2D ), static_cast<float>( mCenterZ2D ), 0.0f ),
			DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f )
		),
		DirectX::XMMatrixOrthographicRH( mViewportSide->GetWidth() * mZoomScale2D, mViewportSide->GetHeight() * mZoomScale2D, 1.0f, 20000.0f )
	);
	mWireframeEffect->Apply( inDeviceContext );

	mWireframeBatch->Begin();

	float minY = static_cast<float>( mViewportSide->GetHeight() * -mZoomScale2D / 2 + mCenterY2D );
	float maxY = static_cast<float>( mViewportSide->GetHeight() * mZoomScale2D / 2 + mCenterY2D );

	float minZ = static_cast<float>( mViewportSide->GetWidth() * -mZoomScale2D / 2 + mCenterZ2D );
	float maxZ = static_cast<float>( mViewportSide->GetWidth() * mZoomScale2D / 2 + mCenterZ2D );

	for ( float zLine = std::round( minZ / mSubGridSize ) * mSubGridSize; zLine <= maxZ; zLine += mSubGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( 0.0f, minY, zLine, 0.0f ), DirectX::ColorsLinear::DimGray },
			{ DirectX::XMVectorSet( 0.0f, maxY, zLine, 0.0f ), DirectX::ColorsLinear::DimGray }
		);
	}

	for ( float yLine = std::round( minY / mSubGridSize ) * mSubGridSize; yLine <= maxY; yLine += mSubGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( 0.0f, yLine, minZ, 0.0f ), DirectX::ColorsLinear::DimGray },
			{ DirectX::XMVectorSet( 0.0f, yLine, maxZ, 0.0f ), DirectX::ColorsLinear::DimGray }
		);
	}

	for ( float zLine = std::round( minZ / mGridSize ) * mGridSize; zLine <= maxZ; zLine += mGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( 0.0f, minY, zLine, 0.0f ), DirectX::Colors::DimGray },
			{ DirectX::XMVectorSet( 0.0f, maxY, zLine, 0.0f ), DirectX::Colors::DimGray }
		);
	}

	for ( float yLine = std::round( minY / mGridSize ) * mGridSize; yLine <= maxY; yLine += mGridSize ) {
		mWireframeBatch->DrawLine(
			{ DirectX::XMVectorSet( 0.0f, yLine, minZ, 0.0f ), DirectX::Colors::DimGray },
			{ DirectX::XMVectorSet( 0.0f, yLine, maxZ, 0.0f ), DirectX::Colors::DimGray }
		);
	}

	mWireframeBatch->DrawLine(
		{ DirectX::XMVectorSet( 0.0f, 0.0f, minZ, 0.0f ), DirectX::Colors::DarkBlue },
		{ DirectX::XMVectorSet( 0.0f, 0.0f, maxZ, 0.0f ), DirectX::Colors::DarkBlue }
	);

	mWireframeBatch->DrawLine(
		{ DirectX::XMVectorSet( 0.0f, minY, 0.0f, 0.0f ), DirectX::Colors::Green },
		{ DirectX::XMVectorSet( 0.0f, maxY, 0.0f, 0.0f ), DirectX::Colors::Green }
	);
	mWireframeBatch->End();

	mViewportSide->Resolve( inDeviceContext );
}
