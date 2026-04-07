#pragma once

#include "pch.h"
#include "Cyclone/Rendering/Shader/WireframeBoxShader.hpp"

// DX Includes
#include <ReadData.h>
#include <DirectXHelpers.h>

const D3D11_INPUT_ELEMENT_DESC Cyclone::Rendering::Shader::WireframeBoxShader::sInputElements[5] = {
	{ "SV_Position",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
	{ "InstCenter",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, 0,	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "InstExtent",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, 16,	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "InstColor",		0, DXGI_FORMAT_R32G32B32_FLOAT,		1, 32,	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "InstEntityID",	0, DXGI_FORMAT_R32_UINT,			1, 44,	D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

void Cyclone::Rendering::Shader::WireframeBoxShader::SetDevice( ID3D11Device *inDevice )
{
	mViewProjBuffer.Create( inDevice );

	CD3D11_BUFFER_DESC instanceDesc(
		kBatchSize * sizeof( InstanceBuffer ),
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	);
	DX::ThrowIfFailed( inDevice->CreateBuffer( &instanceDesc, nullptr, mInstanceBuffer.ReleaseAndGetAddressOf() ) );
	mInstanceData = std::make_unique<InstanceBuffer[]>( kBatchSize );

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
		sInputElements,
		std::size( sInputElements ),
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

	mInstanceCount = 0;
}

void Cyclone::Rendering::Shader::WireframeBoxShader::SetMesh( ID3D11DeviceContext *inContext, const Primitives *inPrimitives )
{
	ID3D11Buffer *vertexBuffer = nullptr;
	ID3D11Buffer *indexBuffer = nullptr;
	inPrimitives->GetShape( kPrimitiveType, kPrimitiveShape, &vertexBuffer, &indexBuffer );

	const UINT vertexStride = PrimitiveTraits::kVertexStride;
	const UINT vertexOffset = 0;

	const UINT instanceStride = sizeof( InstanceBuffer );
	const UINT instanceOffset = 0;

	inContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &vertexStride, &vertexOffset );
	inContext->IASetVertexBuffers( 1, 1, mInstanceBuffer.GetAddressOf(), &instanceStride, &instanceOffset );
	inContext->IASetIndexBuffer( indexBuffer, PrimitiveTraits::kIndexFormat, 0 );
}

void Cyclone::Rendering::Shader::WireframeBoxShader::DrawInstances( ID3D11DeviceContext *inContext )
{
	{
		DirectX::MapGuard map( inContext, mInstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0 );
		memcpy( map.pData, mInstanceData.get(), mInstanceCount * sizeof( InstanceBuffer ) );
	}
	inContext->DrawIndexedInstanced( PrimitiveTraits::kIndexCount, mInstanceCount, 0, 0, 0 );
	mInstanceCount = 0;
}

void XM_CALLCONV Cyclone::Rendering::Shader::WireframeBoxShader::SetViewProj( ID3D11DeviceContext *inContext, DirectX::FXMMATRIX inView, DirectX::FXMMATRIX inProj )
{
	ViewProjBuffer viewProj{ DirectX::XMMatrixMultiplyTranspose( inView, inProj ) };
	mViewProjBuffer.SetData( inContext, viewProj );
	auto buffer = mViewProjBuffer.GetBuffer();
	inContext->VSSetConstantBuffers( 0, 1, &buffer );
}

void XM_CALLCONV Cyclone::Rendering::Shader::WireframeBoxShader::SetInstance( ID3D11DeviceContext *inContext, DirectX::FXMVECTOR inCenter, DirectX::FXMVECTOR inExtent, DirectX::FXMVECTOR inColor, uint32_t inEntityID )
{
	auto &data = mInstanceData[mInstanceCount++];
	data.gCenter = inCenter;
	data.gExtent = inExtent;
	DirectX::XMStoreFloat3( &data.gColor, inColor );
	data.gEntityID = static_cast<uint32_t>( -1 ) - inEntityID;
	if ( mInstanceCount >= kBatchSize ) DrawInstances( inContext );
}
