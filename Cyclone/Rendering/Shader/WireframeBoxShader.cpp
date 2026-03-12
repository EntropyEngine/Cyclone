#pragma once

#include "pch.h"
#include "Cyclone/Rendering/Shader/WireframeBoxShader.hpp"

// DX Includes
#include <ReadData.h>

void Cyclone::Rendering::Shader::WireframeBoxShader::SetDevice( ID3D11Device *inDevice )
{
	mViewProjBuffer.Create( inDevice );

	const auto vsData = DX::ReadData( L"WireFrameBox_VS.cso" );
	DX::ThrowIfFailed( inDevice->CreateVertexShader(
		vsData.data(),
		vsData.size(),
		nullptr,
		mVertexShader.ReleaseAndGetAddressOf()
	) );

	const auto psData = DX::ReadData( L"WireFrameBox_PS.cso" );
	DX::ThrowIfFailed( inDevice->CreatePixelShader(
		psData.data(),
		psData.size(),
		nullptr,
		mPixelShader.ReleaseAndGetAddressOf()
	) );

	DX::ThrowIfFailed( inDevice->CreateInputLayout(
		PrimitiveTraits::VertexType::InputElements,
		PrimitiveTraits::VertexType::InputElementCount,
		vsData.data(),
		vsData.size(),
		mInputLayout.ReleaseAndGetAddressOf()
	) );
}

void Cyclone::Rendering::Shader::WireframeBoxShader::Apply( ID3D11DeviceContext *inContext )
{
	inContext->IASetPrimitiveTopology( PrimitiveTraits::kTopology );
	inContext->IASetInputLayout( mInputLayout.Get() );
	inContext->VSSetShader( mVertexShader.Get(), nullptr, 0 );
	inContext->PSSetShader( mPixelShader.Get(), nullptr, 0 );
}

void Cyclone::Rendering::Shader::WireframeBoxShader::SetMesh( ID3D11DeviceContext *inContext, const Primitives *inPrimitives )
{
	ID3D11Buffer *vertexBuffer = nullptr;
	ID3D11Buffer *indexBuffer = nullptr;
	inPrimitives->GetShape( kPrimitiveType, kPrimitiveShape, &vertexBuffer, &indexBuffer );

	const UINT vertexStride = PrimitiveTraits::kVertexStride;
	const UINT vertexOffset = 0;

	inContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &vertexStride, &vertexOffset );
	inContext->IASetIndexBuffer( indexBuffer, PrimitiveTraits::kIndexFormat, 0 );
}

void Cyclone::Rendering::Shader::WireframeBoxShader::DrawInstance( ID3D11DeviceContext *inContext )
{
	inContext->DrawIndexed( PrimitiveTraits::kIndexCount, 0, 0 );
}

void XM_CALLCONV Cyclone::Rendering::Shader::WireframeBoxShader::SetViewProj( ID3D11DeviceContext *inContext, DirectX::FXMMATRIX inView, DirectX::FXMMATRIX inProj )
{
	ViewProjBuffer viewProj{ DirectX::XMMatrixMultiplyTranspose( inView, inProj ) };
	mViewProjBuffer.SetData( inContext, viewProj );
	auto buffer = mViewProjBuffer.GetBuffer();
	inContext->VSSetConstantBuffers( 0, 1, &buffer );
}

void XM_CALLCONV Cyclone::Rendering::Shader::WireframeBoxShader::SetInstance( ID3D11DeviceContext *inContext, const Cyclone::Core::Component::RenderingBoundingBox &inInstance )
{	
	auto buffer = inInstance.mBuffer.GetBuffer();
	inContext->VSSetConstantBuffers( 1, 1, &buffer );
}
