#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Common includes
#include <MSAAHelper.h>
#include <RenderTexture.h>

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

namespace Cyclone::UI
{
	class ViewportElement : public Cyclone::Util::NonCopyable
	{
	public:
		ViewportElement( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor );
		virtual ~ViewportElement();

		void SetDevice( ID3D11Device3 *inDevice );

		ID3D11ShaderResourceView *GetOrResizeSRV( size_t inWidth, size_t inHeight );
		void Clear( ID3D11DeviceContext3 *inDeviceContext );
		void Resolve( ID3D11DeviceContext3 *inDeviceContext );

		size_t GetWidth() const  { return mWidth; }
		size_t GetHeight() const { return mHeight; }

	protected:
		std::unique_ptr<DX::MSAAHelper> mTargetMSAA;
		std::unique_ptr<DX::RenderTexture> mTargetRT;

		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> mWireframeGridBatch;
		std::unique_ptr<DirectX::BasicEffect>	  mWireframeGridEffect;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mWireframeGridInputLayout;
		std::unique_ptr<DirectX::CommonStates>	  mCommonStates;

		size_t mWidth;
		size_t mHeight;

		DirectX::XMVECTORF32 mClearColor;
	};
}