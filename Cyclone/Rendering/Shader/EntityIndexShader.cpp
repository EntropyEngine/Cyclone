#include "pch.h"
#include "Cyclone/Rendering/Shader/EntityIndexShader.hpp"

// DX Includes
#include <ReadData.h>
#include <DirectXHelpers.h>

void Cyclone::Rendering::Shader::EntityIndexShader::SetDevice( ID3D11Device *inDevice )
{
	mDevice = inDevice;

	mScreenData.Create( inDevice );

	const auto csData1x = DX::ReadData( L"EntityIndex_CS_1x.cso" );
	DX::ThrowIfFailed( inDevice->CreateComputeShader(
		csData1x.data(),
		csData1x.size(),
		nullptr,
		mShader1x.ReleaseAndGetAddressOf()
	) );

	const auto csData4x = DX::ReadData( L"EntityIndex_CS_4x.cso" );
	DX::ThrowIfFailed( inDevice->CreateComputeShader(
		csData4x.data(),
		csData4x.size(),
		nullptr,
		mShader4x.ReleaseAndGetAddressOf()
	) );
}

void Cyclone::Rendering::Shader::EntityIndexShader::SizeResources( size_t inWidth, size_t inHeight, size_t inSamples )
{
	if( mWidth == 0 || mHeight == 0 || inSamples != mSampleCount ) {
		mSampleCount = inSamples;

		D3D11_BUFFER_DESC bufDesc = {};
		bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		bufDesc.StructureByteStride = sizeof( UINT );
		bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufDesc.Usage = D3D11_USAGE_DEFAULT;
		bufDesc.CPUAccessFlags = 0;

		bufDesc.ByteWidth = bufDesc.StructureByteStride * mSampleCount * 8 * 8;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;

		uavDesc.Buffer.NumElements = mSampleCount * 8 * 8;

		D3D11_BUFFER_DESC stagingDesc = {};
		stagingDesc.BindFlags = 0;
		stagingDesc.StructureByteStride = sizeof( UINT );
		stagingDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		stagingDesc.Usage = D3D11_USAGE_STAGING;
		stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		stagingDesc.ByteWidth = bufDesc.ByteWidth;

		DX::ThrowIfFailed( mDevice->CreateBuffer( &bufDesc, nullptr, mOutputBuffer.ReleaseAndGetAddressOf() ) );
		DX::ThrowIfFailed( mDevice->CreateUnorderedAccessView( mOutputBuffer.Get(), &uavDesc, mOutputBufferUAV.ReleaseAndGetAddressOf() ) );
		DX::ThrowIfFailed( mDevice->CreateBuffer( &stagingDesc, nullptr, mOutputBufferStaging.ReleaseAndGetAddressOf() ) );
	}

	if ( inWidth != mWidth || inHeight != mHeight ) {
		mWidth = inWidth;
		mHeight = inHeight;
	}
}

entt::entity Cyclone::Rendering::Shader::EntityIndexShader::ReadViewport( ID3D11DeviceContext *inContext, ID3D11ShaderResourceView *inEntitySRV, size_t inMouseX, size_t inMouseY )
{
	return entt::null;
}
