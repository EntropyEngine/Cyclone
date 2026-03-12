#include "pch.h"
#include "Cyclone/Rendering/Primitives.hpp"

#include <BufferHelpers.h>

void Cyclone::Rendering::Primitives::Initialize( ID3D11Device *inDevice )
{
	CreateBox( inDevice );
}

void Cyclone::Rendering::Primitives::Reset()
{
	for ( auto &type : mVertexBuffers ) {
		for ( auto &shape : type ) {
			shape.Reset();
		}
	}

	for ( auto &type : mIndexBuffers ) {
		for ( auto &shape : type ) {
			shape.Reset();
		}
	}
}

void Cyclone::Rendering::Primitives::CreateBox( ID3D11Device *inDevice )
{
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

	static const WORD s_indices16[24] =
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

	{
		using WireframeLines = PrimitiveTypeTraits<EPrimitiveType::WireframeLines>;

		WireframeLines::VertexType verts[8];
		for ( size_t i = 0; i < 8; ++i ) {
			DirectX::XMStoreFloat3( &verts[i].position, s_verts[i] );
		}

		DX::ThrowIfFailed( DirectX::CreateStaticBuffer( inDevice, verts, 8, D3D11_BIND_VERTEX_BUFFER, GetVertexBuffer( EPrimitiveType::WireframeLines, EPrimitiveShape::Box ).ReleaseAndGetAddressOf() ) );
		DX::ThrowIfFailed( DirectX::CreateStaticBuffer( inDevice, s_indices16, 24, D3D11_BIND_INDEX_BUFFER, GetIndexBuffer( EPrimitiveType::WireframeLines, EPrimitiveShape::Box ).ReleaseAndGetAddressOf() ) );
	}
}
