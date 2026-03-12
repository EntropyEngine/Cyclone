#pragma once

#include "Cyclone/Core/Component/BoundingBox.hpp"

// DX Includes
#include <BufferHelpers.h>

namespace Cyclone::Core::Component
{
	struct RenderingBoundingBox
	{
		RenderingBoundingBox( ID3D11Device *inDevice ) : mBuffer( inDevice ) {}

		struct InstanceBuffer
		{
			DirectX::XMMATRIX gWorld;
			DirectX::XMVECTOR gColor;
		};

		DirectX::ConstantBuffer<InstanceBuffer> mBuffer;

		void XM_CALLCONV Update( ID3D11DeviceContext *inContext, DirectX::FXMVECTOR inCenter, DirectX::FXMVECTOR inExtent, DirectX::FXMVECTOR inColor )
		{
			InstanceBuffer instance{
				DirectX::XMMatrixMultiplyTranspose(
					DirectX::XMMatrixScalingFromVector( inExtent ),
					DirectX::XMMatrixTranslationFromVector( inCenter )
				),
				inColor
			};
			mBuffer.SetData( inContext, instance );
		}
	};

	struct RenderingBoundingBoxPerspective : RenderingBoundingBox {};
	struct RenderingBoundingBoxOrthographic : RenderingBoundingBox {};
}