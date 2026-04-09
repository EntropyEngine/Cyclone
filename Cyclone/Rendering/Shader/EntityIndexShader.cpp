#include "pch.h"
#include "Cyclone/Rendering/Shader/EntityIndexShader.hpp"

// DX Includes
#include <ReadData.h>
#include <DirectXHelpers.h>

// STL
#include <format>
#include <ranges>

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

		bufDesc.ByteWidth = bufDesc.StructureByteStride * mSampleCount * kSearchWidth * kSearchWidth;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;

		uavDesc.Buffer.NumElements = mSampleCount * kSearchWidth * kSearchWidth;

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

		mOutputBufferCPU.resize( mSampleCount * kSearchWidth * kSearchWidth, 0 );
	}

	if ( inWidth != mWidth || inHeight != mHeight ) {
		mWidth = inWidth;
		mHeight = inHeight;
	}
}

entt::entity Cyclone::Rendering::Shader::EntityIndexShader::ReadClosestEntity( ID3D11DeviceContext *inContext, ID3D11ShaderResourceView *inEntitySRV, size_t inMouseX, size_t inMouseY )
{
	DispatchAndMap( inContext, inEntitySRV, inMouseX, inMouseY );
	uint32_t best = GetClosest();
	return static_cast<entt::entity>( static_cast<uint32_t>( entt::null ) - best );
}

std::vector<entt::entity> Cyclone::Rendering::Shader::EntityIndexShader::ReadOrderedEntities( ID3D11DeviceContext * inContext, ID3D11ShaderResourceView * inEntitySRV, size_t inMouseX, size_t inMouseY )
{
	DispatchAndMap( inContext, inEntitySRV, inMouseX, inMouseY );
	std::vector<uint32_t> ordered = GetOrdered();
	std::vector<entt::entity> orderedEntities( ordered.size() );
	for ( size_t i = 0; i < ordered.size(); ++i ) {
		orderedEntities[i] = static_cast<entt::entity>( static_cast<uint32_t>( entt::null ) - ordered[i] );
	}
	return orderedEntities;
}



void Cyclone::Rendering::Shader::EntityIndexShader::DispatchAndMap( ID3D11DeviceContext *inContext, ID3D11ShaderResourceView *inEntitySRV, size_t inMouseX, size_t inMouseY )
{
	mScreenData.SetData( inContext, { static_cast<uint32_t>( inMouseX ), static_cast<uint32_t>( inMouseY ), static_cast<uint32_t>( mWidth ), static_cast<uint32_t>( mHeight ) } );
	ID3D11Buffer *screenDataBuffer = mScreenData.GetBuffer();

	ID3D11UnorderedAccessView *outputUAV = mOutputBufferUAV.Get();

	inContext->CSSetConstantBuffers( 0, 1, &screenDataBuffer );
	inContext->CSSetShaderResources( 0, 1, &inEntitySRV );
	inContext->CSSetUnorderedAccessViews( 0, 1, &outputUAV, nullptr );

	switch ( mSampleCount ) {
		case 1: inContext->CSSetShader( mShader1x.Get(), nullptr, 0 ); break;
		case 4: inContext->CSSetShader( mShader4x.Get(), nullptr, 0 ); break;
		default:
			assert( false );
			__assume( false );
	}

	inContext->Dispatch( 1, 1, 1 );

	ID3D11Buffer *clearCB[1] = { nullptr };
	ID3D11ShaderResourceView *clearSRV[1] = { nullptr };
	ID3D11UnorderedAccessView *clearUAV[1] = { nullptr };

	inContext->CSSetConstantBuffers( 0, 1, clearCB );
	inContext->CSSetShaderResources( 0, 1, clearSRV );
	inContext->CSSetUnorderedAccessViews( 0, 1, clearUAV, nullptr );

	inContext->CopyResource( mOutputBufferStaging.Get(), mOutputBuffer.Get() );

	{
		DirectX::MapGuard map( inContext, mOutputBufferStaging.Get(), 0, D3D11_MAP_READ, 0 );
		memcpy( mOutputBufferCPU.data(), map.pData, mOutputBufferCPU.size() * sizeof( uint32_t ) );
	}
}

uint32_t Cyclone::Rendering::Shader::EntityIndexShader::GetClosest() const
{
	uint32_t best = 0;
	float bestDistance = kSearchWidth * kSearchWidth;

	for ( size_t x = 0; x < kSearchWidth; ++x ) {
		for ( size_t y = 0; y < kSearchWidth; ++y ) {
			for ( size_t s = 0; s < mSampleCount; ++s ) {
				uint32_t index = mOutputBufferCPU[x + y * kSearchWidth + s * kSearchWidth * kSearchWidth];
				if ( index != 0 ) {
					float dx = static_cast<float>( x ) - kSearchWidth / 2;
					float dy = static_cast<float>( y ) - kSearchWidth / 2;
					float dist = std::sqrtf( dx * dx + dy * dy );
					if ( dist < bestDistance ) {
						best = index;
						bestDistance = dist;
					}
				}
			}
		}
	}

	return best;
}

std::vector<uint32_t> Cyclone::Rendering::Shader::EntityIndexShader::GetOrdered() const
{
	std::vector<std::pair<uint32_t, float>> counts;
	counts.reserve( 16 );

	for ( size_t x = 0; x < kSearchWidth; ++x ) {
		for ( size_t y = 0; y < kSearchWidth; ++y ) {
			for ( size_t s = 0; s < mSampleCount; ++s ) {
				uint32_t index = mOutputBufferCPU[x + y * kSearchWidth + s * kSearchWidth * kSearchWidth];
				if ( index != 0 ) {
					float dx = static_cast<float>( x ) - kSearchWidth / 2;
					float dy = static_cast<float>( y ) - kSearchWidth / 2;
					float dist = std::sqrtf( dx * dx + dy * dy );
					
					auto it = std::find_if( counts.begin(), counts.end(), [index]( const auto &inPair ){ return inPair.first == index; } );
					if ( it == counts.end() ) {
						counts.emplace_back( index, dist );
					}
					else {
						it->second = std::min( it->second, dist );
					}
				}
			}
		}
	}

	std::sort( counts.begin(), counts.end(), []( const auto &inLhs, const auto &inRhs ) { return inLhs.second < inRhs.second; } );

	std::vector<uint32_t> sorted( counts.size() );

	for ( size_t i = 0; i < counts.size(); ++i ) {
		sorted[i] = counts[i].first;
	}
	
	return sorted;
}
