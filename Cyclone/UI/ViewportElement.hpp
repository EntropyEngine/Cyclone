#pragma once

// Common includes
#include <MSAAHelper.h>
#include <RenderTexture.h>

namespace Cyclone
{
	namespace UI
	{
		class ViewportElement
		{
		public:
			ViewportElement( ID3D11Device3 *inDevice, DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor );

			ViewportElement( ViewportElement && ) = default;
			ViewportElement &operator= ( ViewportElement && ) = default;

			ViewportElement( ViewportElement const & ) = delete;
			ViewportElement &operator= ( ViewportElement const & ) = delete;

			ID3D11ShaderResourceView *GetImageSRV( size_t inWidth, size_t inHeight );
			void Clear( ID3D11DeviceContext3 *inDeviceContext );
			void Resolve( ID3D11DeviceContext3 *inDeviceContext );
		protected:
			std::unique_ptr<DX::MSAAHelper> mTargetMSAA;
			std::unique_ptr<DX::RenderTexture> mTargetRT;

			size_t mWidth;
			size_t mHeight;

			DirectX::XMVECTORF32 mClearColor;
		};
	}
}