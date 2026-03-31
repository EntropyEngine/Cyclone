#include "pch.h"

#include "Cyclone/UI/ViewportElement.hpp"

// DX Includes
#include <DirectXHelpers.h>

Cyclone::UI::ViewportElement::ViewportElement( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor, bool inAntialiasing )
{
	mTargetMSAA = std::make_unique<DX::MSAAHelper>( inBackBufferFormat, inDepthBufferFormat, inAntialiasing ? 4 : 1 );
	mTargetRT = std::make_unique<DX::RenderTexture>( inBackBufferFormat );
	mClearColor = inClearColor;

	mWireframeBoxShader = std::make_unique<Cyclone::Rendering::Shader::WireframeBoxShader>();

	mWidth = 0;
	mHeight = 0;
}

Cyclone::UI::ViewportElement::~ViewportElement()
{
	mTargetMSAA->ReleaseDevice();
	mTargetRT->ReleaseDevice();
}

void Cyclone::UI::ViewportElement::SetDevice( ID3D11Device3 *inDevice )
{
	mTargetMSAA->SetDevice( inDevice );
	mTargetRT->SetDevice( inDevice );

	mWireframeBoxShader->SetDevice( inDevice );

	CD3D11_RASTERIZER_DESC rssDesc( D3D11_DEFAULT );
	rssDesc.FillMode = D3D11_FILL_WIREFRAME;
	rssDesc.CullMode = D3D11_CULL_NONE;
	rssDesc.FrontCounterClockwise = TRUE;
	DX::ThrowIfFailed( inDevice->CreateRasterizerState( &rssDesc, mWireframeRSS.ReleaseAndGetAddressOf() ) );

	rssDesc.DepthBias = -1;
	rssDesc.MultisampleEnable = TRUE;
	DX::ThrowIfFailed( inDevice->CreateRasterizerState( &rssDesc, mToolRSS_MSAA.ReleaseAndGetAddressOf() ) );

	rssDesc.DepthBias = -1;
	rssDesc.MultisampleEnable = FALSE;
	DX::ThrowIfFailed( inDevice->CreateRasterizerState( &rssDesc, mToolRSS_NOAA.ReleaseAndGetAddressOf() ) );

	mCommonStates = std::make_unique<DirectX::CommonStates>( inDevice );

	mWireframeGridEffect = std::make_unique<DirectX::BasicEffect>( inDevice );
	mWireframeGridEffect->SetVertexColorEnabled( true );
	DX::ThrowIfFailed( DirectX::CreateInputLayoutFromEffect<DirectX::VertexPositionColor>( inDevice, mWireframeGridEffect.get(), mWireframeGridInputLayout.ReleaseAndGetAddressOf() ) );

	Microsoft::WRL::ComPtr<ID3D11DeviceContext3> deviceContext;
	inDevice->GetImmediateContext3( deviceContext.GetAddressOf() );
	mWireframeGridBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>( deviceContext.Get() );
}

void Cyclone::UI::ViewportElement::UpdateViewportData()
{
	mViewportData.mViewSize = ImGui::GetWindowSize();
	mViewportData.mViewOrigin = ImGui::GetCursorScreenPos();
	mViewportData.mDrawList = ImGui::GetWindowDrawList();
	mViewportData.mIsActive = false;

	if ( mTargetMSAA->GetSampleCount() <= 1 ) {
		mViewportData.mDrawList->Flags &= ~( ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedLinesUseTex | ImDrawListFlags_AntiAliasedFill );
	}

	// Split channels into 4 planes
	mViewportData.mDrawList->ChannelsSplit( 4 );
	mViewportData.mDrawList->ChannelsSetCurrent( 0 );
}

ID3D11ShaderResourceView *Cyclone::UI::ViewportElement::GetOrResizeSRV( size_t inWidth, size_t inHeight )
{
	mTargetMSAA->SizeResources( inWidth, inHeight );
	mTargetRT->SizeResources( inWidth, inHeight );

	mWidth = inWidth;
	mHeight = inHeight;

	return mTargetRT->GetShaderResourceView();
}

void Cyclone::UI::ViewportElement::Clear( ID3D11DeviceContext3 * inDeviceContext )
{
	ID3D11RenderTargetView *renderTargetView = mTargetMSAA->GetMSAARenderTargetView();
	ID3D11DepthStencilView *depthStencilView = mTargetMSAA->GetMSAADepthStencilView();

	inDeviceContext->ClearRenderTargetView( renderTargetView, mClearColor );
	inDeviceContext->ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

	inDeviceContext->OMSetRenderTargets( 1, &renderTargetView, depthStencilView );

	D3D11_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>( mWidth ), static_cast<float>( mHeight ), 0.0f, 1.0f };
	inDeviceContext->RSSetViewports( 1, &viewport );
}

void Cyclone::UI::ViewportElement::Resolve( ID3D11DeviceContext3 * inDeviceContext )
{
	mTargetMSAA->Resolve( inDeviceContext, mTargetRT->GetRenderTarget() );
}

void Cyclone::UI::ViewportElement::ToggleAntialiasing( bool inEnabled )
{
	mTargetMSAA->SetSampleCount( inEnabled ? 4 : 1 );
}
