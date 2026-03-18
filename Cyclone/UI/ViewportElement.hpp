#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone UI
#include "Cyclone/UI/ViewportData.hpp"

// Common includes
#include <MSAAHelper.h>
#include <RenderTexture.h>

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

// Rendering includes
#include "Cyclone/Rendering/Shader/WireframeBoxShader.hpp"


namespace Cyclone::UI
{
	class ViewportElement : public Cyclone::Util::NonCopyable
	{
	public:
		ViewportElement( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor, bool inAntialiasing );
		virtual ~ViewportElement();

		void SetDevice( ID3D11Device3 *inDevice );
		void UpdateViewportData();
		const ViewportData &GetViewportData() const { return mViewportData; }

		ID3D11ShaderResourceView *GetOrResizeSRV( size_t inWidth, size_t inHeight );
		void Clear( ID3D11DeviceContext3 *inDeviceContext );
		void Resolve( ID3D11DeviceContext3 *inDeviceContext );

		size_t GetWidth() const  { return mWidth; }
		size_t GetHeight() const { return mHeight; }

		void ToggleAntialiasing( bool inEnabled );

	protected:
		std::unique_ptr<DX::MSAAHelper>				mTargetMSAA;
		std::unique_ptr<DX::RenderTexture>			mTargetRT;

		std::unique_ptr<Cyclone::Rendering::Shader::WireframeBoxShader> mWireframeBoxShader;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> mWireframeRSS;

		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> mWireframeGridBatch;
		std::unique_ptr<DirectX::BasicEffect>		mWireframeGridEffect;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	mWireframeGridInputLayout;
		std::unique_ptr<DirectX::CommonStates>		mCommonStates;

		ViewportData								mViewportData;

		size_t										mWidth;
		size_t										mHeight;

		DirectX::XMVECTORF32						mClearColor;
	};
}