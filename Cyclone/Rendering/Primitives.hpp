#pragma once

#include <VertexTypes.h>

namespace Cyclone::Rendering
{
	using Microsoft::WRL::ComPtr;

	enum class EPrimitiveShape
	{
		Box,
		_Count
	};

	enum class EPrimitiveType
	{
		WireframeLines,		///< Wireframe primitive constructed from line lists. Useful for n-gons
		_Count,
	};

	template<EPrimitiveType T>
	struct PrimitiveTypeTraits;

	template<>
	struct PrimitiveTypeTraits<EPrimitiveType::WireframeLines>
	{
		using VertexType = DirectX::VertexPosition;
		static constexpr UINT kVertexStride = sizeof( VertexType );
		static constexpr DXGI_FORMAT kIndexFormat = DXGI_FORMAT_R16_UINT;
		static constexpr D3D_PRIMITIVE_TOPOLOGY kTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	};

	class Primitives
	{
	public:
		Primitives() = default;

		void Initialize( ID3D11Device *inDevice );

		void GetShape( EPrimitiveType inType, EPrimitiveShape inShape, ID3D11Buffer **outVertex, ID3D11Buffer **outIndex ) const
		{
			*outVertex = GetVertexBuffer( inType, inShape ).Get();
			*outIndex = GetIndexBuffer( inType, inShape ).Get();
		}

		void Reset();

	protected:
		static constexpr size_t kShapeCount = static_cast<size_t>( EPrimitiveShape::_Count );
		static constexpr size_t kTypeCount = static_cast<size_t>( EPrimitiveType::_Count );

		std::array<std::array<ComPtr<ID3D11Buffer>, kShapeCount>, kTypeCount> mVertexBuffers;
		std::array<std::array<ComPtr<ID3D11Buffer>, kShapeCount>, kTypeCount> mIndexBuffers;

		const ComPtr<ID3D11Buffer> & GetVertexBuffer( EPrimitiveType inType, EPrimitiveShape inShape ) const { return mVertexBuffers[static_cast<size_t>( inType )][static_cast<size_t>( inShape )]; }
		const ComPtr<ID3D11Buffer> & GetIndexBuffer( EPrimitiveType inType, EPrimitiveShape inShape ) const { return mIndexBuffers[static_cast<size_t>( inType )][static_cast<size_t>( inShape )]; }

		ComPtr<ID3D11Buffer> & GetVertexBuffer( EPrimitiveType inType, EPrimitiveShape inShape ) { return mVertexBuffers[static_cast<size_t>( inType )][static_cast<size_t>( inShape )]; }
		ComPtr<ID3D11Buffer> & GetIndexBuffer( EPrimitiveType inType, EPrimitiveShape inShape ) { return mIndexBuffers[static_cast<size_t>( inType )][static_cast<size_t>( inShape )]; }

		void CreateBox( ID3D11Device *inDevice );
	};
}