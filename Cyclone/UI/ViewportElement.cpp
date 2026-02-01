#include "pch.h"

#include "Cyclone/UI/ViewportElement.hpp"

Cyclone::UI::ViewportElement::ViewportElement( ID3D11Device3 *inDevice, DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor )
{
	mTargetMSAA = std::make_unique<DX::MSAAHelper>( inBackBufferFormat, inDepthBufferFormat );
	mTargetRT = std::make_unique<DX::RenderTexture>( inBackBufferFormat );
	mClearColor = inClearColor;

	mTargetMSAA->SetDevice( inDevice );
	mTargetRT->SetDevice( inDevice );

	mWidth = 0;
	mHeight = 0;
}

ID3D11ShaderResourceView *Cyclone::UI::ViewportElement::GetImageSRV( size_t inWidth, size_t inHeight )
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
