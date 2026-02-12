#pragma once

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>

namespace Cyclone::Util
{
	inline void XM_CALLCONV RenderWireframeBoundingBox( DirectX::PrimitiveBatch<DirectX::VertexPositionColor> *inBatch, DirectX::FXMVECTOR inRebasedBoxCenter, DirectX::FXMVECTOR inBoxExtent, DirectX::FXMVECTOR inBoxColor )
	{
		DirectX::XMMATRIX matWorld = DirectX::XMMatrixScalingFromVector( inBoxExtent );
		matWorld.r[3] = DirectX::XMVectorSelect( matWorld.r[3], inRebasedBoxCenter, DirectX::g_XMSelect1110 );

		static const DirectX::XMVECTORF32 s_verts[8] =
		{
			{ { { -1.f, -1.f, -1.f, 0.f } } },
			{ { {  1.f, -1.f, -1.f, 0.f } } },
			{ { {  1.f, -1.f,  1.f, 0.f } } },
			{ { { -1.f, -1.f,  1.f, 0.f } } },
			{ { { -1.f,  1.f, -1.f, 0.f } } },
			{ { {  1.f,  1.f, -1.f, 0.f } } },
			{ { {  1.f,  1.f,  1.f, 0.f } } },
			{ { { -1.f,  1.f,  1.f, 0.f } } }
		};

		static const WORD s_indices[] =
		{
			0, 1,
			1, 2,
			2, 3,
			3, 0,
			4, 5,
			5, 6,
			6, 7,
			7, 4,
			0, 4,
			1, 5,
			2, 6,
			3, 7
		};

		DirectX::VertexPositionColor verts[8];

		for (size_t i = 0; i < 8; ++i) {
			DirectX::XMVECTOR v = DirectX::XMVector3Transform( s_verts[i], matWorld );
			DirectX::XMStoreFloat3( &verts[i].position, v );
			DirectX::XMStoreFloat4( &verts[i].color, inBoxColor );
		}

		inBatch->DrawIndexed( D3D_PRIMITIVE_TOPOLOGY_LINELIST, s_indices, static_cast<UINT>( std::size( s_indices ) ), verts, 8 );
	}
}