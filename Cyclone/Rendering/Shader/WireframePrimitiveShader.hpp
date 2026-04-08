#pragma once

// DX Includes
#include <BufferHelpers.h>

namespace Cyclone::Rendering::Shader
{
	using Microsoft::WRL::ComPtr;

	class WireframePrimitiveShader
	{
	public:
		WireframePrimitiveShader() = default;

		void SetDevice( ID3D11Device *inDevice );

		void Apply( ID3D11DeviceContext *inContext );

		void XM_CALLCONV SetViewProj( ID3D11DeviceContext *inContext, DirectX::FXMMATRIX inView, DirectX::FXMMATRIX inProj );

		struct VertexPositionColorID
		{
			DirectX::XMFLOAT3 PositionPS;
			DirectX::XMFLOAT3 Color;
			uint32_t		  EntityID;

			VertexPositionColorID() = default;
			VertexPositionColorID( DirectX::FXMVECTOR inPosition, DirectX::FXMVECTOR inColor, entt::entity inEntity )
			{
				DirectX::XMStoreFloat3( &PositionPS, inPosition );
				DirectX::XMStoreFloat3( &Color, inColor );
				EntityID = static_cast<uint32_t>( -1 ) - static_cast<uint32_t>( inEntity );
			}
		};

	protected:
		ComPtr<ID3D11VertexShader>	mVertexShader;
		ComPtr<ID3D11PixelShader>	mPixelShader;
		ComPtr<ID3D11InputLayout>	mInputLayout;

		struct ViewProjBuffer
		{
			DirectX::XMMATRIX gViewProj;
		};
		DirectX::ConstantBuffer<ViewProjBuffer> mViewProjBuffer;

		static const D3D11_INPUT_ELEMENT_DESC sInputElements[3];
	};
}