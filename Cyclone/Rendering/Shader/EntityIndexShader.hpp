#pragma once

// DX Includes
#include <BufferHelpers.h>

namespace Cyclone::Rendering::Shader
{
	using Microsoft::WRL::ComPtr;

	class EntityIndexShader
	{
	public:
		void SetDevice( ID3D11Device *inDevice );
		void SizeResources( size_t inWidth, size_t inHeight, size_t inSamples );

		entt::entity ReadViewport( ID3D11DeviceContext *inContext, ID3D11ShaderResourceView *inEntitySRV, size_t inMouseX, size_t inMouseY );

	protected:
		ComPtr<ID3D11Device>	mDevice;

		size_t					mWidth = 0;
		size_t					mHeight = 0;
		size_t					mSampleCount = 0;

		struct ScreenData
		{
			uint32_t gMouseX;
			uint32_t gMouseY;
			uint32_t gScreenW;
			uint32_t gScreenH;
		};

		DirectX::ConstantBuffer<ScreenData> mScreenData;

		ComPtr<ID3D11Buffer>				mOutputBuffer;
		ComPtr<ID3D11UnorderedAccessView>	mOutputBufferUAV;
		ComPtr<ID3D11Buffer>				mOutputBufferStaging;

		ComPtr<ID3D11ComputeShader>			mShader1x;
		ComPtr<ID3D11ComputeShader>			mShader4x;
	};
}