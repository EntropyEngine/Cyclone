
#include "pch.h"
#include "Cyclone/Rendering/Shader/WireframePrimitiveShader.hpp"

// DX Includes
#include <ReadData.h>
#include <DirectXHelpers.h>

const D3D11_INPUT_ELEMENT_DESC Cyclone::Rendering::Shader::WireframePrimitiveShader::sInputElements[3] = {
	{ "SV_Position",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
	{ "COLOR",			0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
	{ "EntityID",		0, DXGI_FORMAT_R32_UINT,		0, 24,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
};

void Cyclone::Rendering::Shader::WireframePrimitiveShader::SetDevice( ID3D11Device *inDevice )
{
	mViewProjBuffer.Create( inDevice );

	const auto vsData = DX::ReadData( L"WireframePrimitive_VS.cso" );
	DX::ThrowIfFailed( inDevice->CreateVertexShader(
		vsData.data(),
		vsData.size(),
		nullptr,
		mVertexShader.ReleaseAndGetAddressOf()
	) );

	const auto psData = DX::ReadData( L"WireframePrimitive_PS.cso" );
	DX::ThrowIfFailed( inDevice->CreatePixelShader(
		psData.data(),
		psData.size(),
		nullptr,
		mPixelShader.ReleaseAndGetAddressOf()
	) );

	DX::ThrowIfFailed( inDevice->CreateInputLayout(
		sInputElements,
		std::size( sInputElements ),
		vsData.data(),
		vsData.size(),
		mInputLayout.ReleaseAndGetAddressOf()
	) );
}

void Cyclone::Rendering::Shader::WireframePrimitiveShader::Apply( ID3D11DeviceContext *inContext )
{
	inContext->IASetInputLayout( mInputLayout.Get() );
	inContext->VSSetShader( mVertexShader.Get(), nullptr, 0 );
	inContext->PSSetShader( mPixelShader.Get(), nullptr, 0 );
}

void XM_CALLCONV Cyclone::Rendering::Shader::WireframePrimitiveShader::SetViewProj( ID3D11DeviceContext *inContext, DirectX::FXMMATRIX inView, DirectX::FXMMATRIX inProj )
{
	ViewProjBuffer viewProj{ DirectX::XMMatrixMultiplyTranspose( inView, inProj ) };
	mViewProjBuffer.SetData( inContext, viewProj );
	auto buffer = mViewProjBuffer.GetBuffer();
	inContext->VSSetConstantBuffers( 0, 1, &buffer );
}