#pragma once

// Cyclone Rendering
#include "Cyclone/Rendering/Primitives.hpp"

// DX Includes
#include <VertexTypes.h>
#include <BufferHelpers.h>

namespace Cyclone::Rendering::Shader
{
	using Microsoft::WRL::ComPtr;

	class WireframeBoxShader
	{
	public:
		static constexpr auto kPrimitiveType = EPrimitiveType::WireframeLines;
		static constexpr auto kPrimitiveShape = EPrimitiveShape::Box;
		using PrimitiveTraits = PrimitiveTypeTraits<kPrimitiveType, kPrimitiveShape>;

		static constexpr UINT kBatchSize = 256;

		WireframeBoxShader() = default;

		void SetDevice( ID3D11Device *inDevice );

		void Apply( ID3D11DeviceContext *inContext );

		void SetMesh( ID3D11DeviceContext *inContext, const Primitives *inPrimitives );
		void DrawInstances( ID3D11DeviceContext *inContext );

		void XM_CALLCONV SetViewProj( ID3D11DeviceContext *inContext, DirectX::FXMMATRIX inView, DirectX::FXMMATRIX inProj );
		void XM_CALLCONV SetInstance( ID3D11DeviceContext *inContext, DirectX::FXMVECTOR inCenter, DirectX::FXMVECTOR inExtent, DirectX::FXMVECTOR inColor, uint32_t inEntityID );

	protected:
		ComPtr<ID3D11VertexShader>	mVertexShader;
		ComPtr<ID3D11PixelShader>	mPixelShader;
		ComPtr<ID3D11InputLayout>	mInputLayout;

		struct ViewProjBuffer
		{
			DirectX::XMMATRIX gViewProj;
		};
		DirectX::ConstantBuffer<ViewProjBuffer> mViewProjBuffer;

		struct InstanceBuffer
		{
			DirectX::XMVECTOR gCenter;
			DirectX::XMVECTOR gExtent;
			DirectX::XMFLOAT3 gColor;
			uint32_t		  gEntityID;
		};
		ComPtr<ID3D11Buffer> mInstanceBuffer;
		std::unique_ptr<InstanceBuffer[]> mInstanceData;
		UINT mInstanceCount;

		static const D3D11_INPUT_ELEMENT_DESC sInputElements[5];
	};
}