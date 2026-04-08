#pragma once

// DX Includes
#include <BufferHelpers.h>

// STL Includes
#include <bit>

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
				EntityID = static_cast<uint32_t>( -1 ) - ( static_cast<uint32_t>( inEntity ) & entt::entt_traits<entt::entity>::entity_mask );
			}

			VertexPositionColorID( DirectX::FXMVECTOR inPosition, DirectX::FXMVECTOR inColor, entt::entity inEntity, uint16_t inIdentifier )
			{
				constexpr int idShift = std::popcount( entt::entt_traits<entt::entity>::entity_mask );
				constexpr int idMax = 1 << std::popcount( entt::entt_traits<entt::entity>::version_mask );
				assert( inIdentifier < idMax );

				uint32_t lower = static_cast<uint32_t>( inEntity ) & entt::entt_traits<entt::entity>::entity_mask;
				uint32_t upper = static_cast<uint32_t>( inIdentifier ) << idShift;

				DirectX::XMStoreFloat3( &PositionPS, inPosition );
				DirectX::XMStoreFloat3( &Color, inColor );
				EntityID = static_cast<uint32_t>( -1 ) - lower - upper;
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