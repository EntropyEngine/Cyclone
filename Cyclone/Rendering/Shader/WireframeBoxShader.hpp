#pragma once

// Cyclone Rendering
#include "Cyclone/Rendering/Primitives.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/RenderingBoundingBox.hpp"

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

		WireframeBoxShader() = default;

		void SetDevice( ID3D11Device *inDevice );

		void Apply( ID3D11DeviceContext *inContext );

		void SetMesh( ID3D11DeviceContext *inContext, const Primitives *inPrimitives );
		void DrawInstance( ID3D11DeviceContext *inContext );

		void XM_CALLCONV SetViewProj( ID3D11DeviceContext *inContext, DirectX::FXMMATRIX inView, DirectX::FXMMATRIX inProj );
		void XM_CALLCONV SetInstance( ID3D11DeviceContext *inContext, const Cyclone::Core::Component::RenderingBoundingBox &inInstance );

	protected:
		ComPtr<ID3D11VertexShader>	mVertexShader;
		ComPtr<ID3D11PixelShader>	mPixelShader;
		ComPtr<ID3D11InputLayout>	mInputLayout;

		struct ViewProjBuffer
		{
			DirectX::XMMATRIX gViewProj;
		};

		DirectX::ConstantBuffer<ViewProjBuffer> mViewProjBuffer;
	};
}