#pragma once

// Cyclone includes
#include "Cyclone/UI/ViewportElement.hpp"

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

namespace Cyclone
{
	namespace UI
	{
		class ViewportManager
		{
		public:
			ViewportManager( ID3D11Device3 *inDevice );

			ViewportManager( ViewportManager && ) = default;
			ViewportManager &operator= ( ViewportManager && ) = default;

			ViewportManager( ViewportManager const & ) = delete;
			ViewportManager &operator= ( ViewportManager const & ) = delete;

			void Update( float inDeltaTime );

			void RenderPerspective( ID3D11DeviceContext3 *inDeviceContext );
			void RenderTop( ID3D11DeviceContext3 *inDeviceContext );
			void RenderFront( ID3D11DeviceContext3 *inDeviceContext );
			void RenderSide( ID3D11DeviceContext3 *inDeviceContext );

		protected:
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportPerspective;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportTop;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportFront;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportSide;

			std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> mWireframeBatch;
			std::unique_ptr<DirectX::BasicEffect>	  mWireframeEffect;
			Microsoft::WRL::ComPtr<ID3D11InputLayout> mWireFrameInputLayout;
			std::unique_ptr<DirectX::CommonStates>	  mCommonStates;

			int mZoomLevel = 0;
			double mZoomScale2D = 0.1; // Pixels to meters

			double mWorldLimit = 10000.0; // World +/- limit in meters
			double mGridSize = 10.0; // Grid spacing in meters
			double mSubGridSize = 1.0; // Grid spacing in meters

			double mMinGridSize = 5.0; // Min subgrid view size in pixels

			double mCenterX2D = 0.0;
			double mCenterY2D = 0.0;
			double mCenterZ2D = 0.0;

			template<EViewportType T>
			void RenderWireFrame( ID3D11DeviceContext3 *inDeviceContext )
			{
				constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
				constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

				ViewportElement *viewport = GetViewport<T>();

				viewport->Clear( inDeviceContext );

				inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
				inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
				inDeviceContext->RSSetState( mCommonStates->CullNone() );
				inDeviceContext->IASetInputLayout( mWireFrameInputLayout.Get() );

				mWireframeEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>(), GetProjMatrix<T>() );
				mWireframeEffect->Apply( inDeviceContext );

				mWireframeBatch->Begin();

				double minU, maxU, minV, maxV;
				GetMinMaxUV<T>( minU, maxU, minV, maxV );

				double subgridStep = mSubGridSize;
				double gridStep = mGridSize;

				if ( subgridStep / mZoomScale2D > mMinGridSize ) {
					DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, subgridStep, DirectX::ColorsLinear::DimGray );
					DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, subgridStep, DirectX::ColorsLinear::DimGray );
				}

				DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, gridStep, DirectX::Colors::DimGray );
				DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, gridStep, DirectX::Colors::DimGray );

				DrawAxisLine<AxisU>( minU, maxU );
				DrawAxisLine<AxisV>( minV, maxV );

				mWireframeBatch->End();

				viewport->Resolve( inDeviceContext );
			}

		private:
			template<EViewportType T>
			void GetMinMaxUV( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV );

			template<> void GetMinMaxUV<EViewportType::TopXZ>( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				ViewportElement *viewport = GetViewport<EViewportType::TopXZ>();

				outMinU = std::max( -mWorldLimit, viewport->GetWidth() * -mZoomScale2D / 2 + mCenterX2D );
				outMaxU = std::min( mWorldLimit, viewport->GetWidth() * mZoomScale2D / 2 + mCenterX2D );

				outMinV = std::max( -mWorldLimit, viewport->GetHeight() * -mZoomScale2D / 2 + mCenterZ2D );
				outMaxV = std::min( mWorldLimit, viewport->GetHeight() * mZoomScale2D / 2 + mCenterZ2D );
			}

			template<> void GetMinMaxUV<EViewportType::FrontXY>( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				ViewportElement *viewport = GetViewport<EViewportType::FrontXY>();

				outMinU = std::max( -mWorldLimit, viewport->GetWidth() * -mZoomScale2D / 2 - mCenterX2D );
				outMaxU = std::min( mWorldLimit, viewport->GetWidth() * mZoomScale2D / 2 - mCenterX2D );

				outMinV = std::max( -mWorldLimit, viewport->GetHeight() * -mZoomScale2D / 2 + mCenterY2D );
				outMaxV = std::min( mWorldLimit, viewport->GetHeight() * mZoomScale2D / 2 + mCenterY2D );
			}

			template<> void GetMinMaxUV<EViewportType::SideYZ>( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				ViewportElement *viewport = GetViewport<EViewportType::SideYZ>();

				outMinU = std::max( -mWorldLimit, viewport->GetHeight() * -mZoomScale2D / 2 + mCenterY2D );
				outMaxU = std::min( mWorldLimit, viewport->GetHeight() * mZoomScale2D / 2 + mCenterY2D );

				outMinV = std::max( -mWorldLimit, viewport->GetWidth() * -mZoomScale2D / 2 + mCenterZ2D );
				outMaxV = std::min( mWorldLimit, viewport->GetWidth() * mZoomScale2D / 2 + mCenterZ2D );
			}


			template<size_t SwizzleFixed, size_t SwizzleLine>
			void DrawLineLoop( double inFixedMin, double inFixedMax, double inLineMin, double inLineMax, double inStep, DirectX::FXMVECTOR inColor )
			{
				DirectX::XMVECTOR vFixedMin = DirectX::XMVectorSetByIndex( DirectX::g_XMZero, static_cast<float>( inFixedMin ), SwizzleFixed );
				DirectX::XMVECTOR vFixedMax = DirectX::XMVectorSetByIndex( DirectX::g_XMZero, static_cast<float>( inFixedMax ), SwizzleFixed );

				for ( double line = std::round( inLineMin / inStep ) * inStep; line <= inLineMax; line += inStep ) {
					mWireframeBatch->DrawLine(
						{ DirectX::XMVectorSetByIndex( vFixedMin, static_cast<float>( line ), SwizzleLine ), inColor },
						{ DirectX::XMVectorSetByIndex( vFixedMax, static_cast<float>( line ), SwizzleLine ), inColor }
					);
				}
			}


			template<size_t Axis>
			void DrawAxisLine( double inMin, double inMax );

			template<> void DrawAxisLine<0>( double inMin, double inMax )
			{
				mWireframeBatch->DrawLine(
					{ DirectX::XMVectorSet( static_cast<float>( inMin ), 0.0f, 0.0f, 0.0f ), DirectX::Colors::DarkRed },
					{ DirectX::XMVectorSet( static_cast<float>( inMax ), 0.0f, 0.0f, 0.0f ), DirectX::Colors::DarkRed }
				);
			}

			template<> void DrawAxisLine<1>( double inMin, double inMax )
			{
				mWireframeBatch->DrawLine(
					{ DirectX::XMVectorSet( 0.0f, static_cast<float>( inMin ), 0.0f, 0.0f ), DirectX::Colors::Green },
					{ DirectX::XMVectorSet( 0.0f, static_cast<float>( inMax ), 0.0f, 0.0f ), DirectX::Colors::Green }
				);
			}

			template<> void DrawAxisLine<2>( double inMin, double inMax )
			{
				mWireframeBatch->DrawLine(
					{ DirectX::XMVectorSet( 0.0f, 0.0f, static_cast<float>( inMin ), 0.0f ), DirectX::Colors::DarkBlue },
					{ DirectX::XMVectorSet( 0.0f, 0.0f, static_cast<float>( inMax ), 0.0f ), DirectX::Colors::DarkBlue }
				);
			}


			template<EViewportType T>
			ViewportElement *GetViewport();

			template<> ViewportElement *GetViewport<EViewportType::Perspective>() { return mViewportPerspective.get(); }
			template<> ViewportElement *GetViewport<EViewportType::TopXZ>()		  { return mViewportTop.get(); }
			template<> ViewportElement *GetViewport<EViewportType::FrontXY>()	  { return mViewportFront.get(); }
			template<> ViewportElement *GetViewport<EViewportType::SideYZ>()	  { return mViewportSide.get(); }


			template<EViewportType T>
			DirectX::XMMATRIX XM_CALLCONV GetViewMatrix() const;

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::TopXZ>() const
			{
				return DirectX::XMMatrixLookAtRH(
					DirectX::XMVectorSet( static_cast<float>( mCenterX2D ), static_cast<float>( mWorldLimit ), static_cast<float>( mCenterZ2D ), 0.0f ),
					DirectX::XMVectorSet( static_cast<float>( mCenterX2D ), static_cast<float>( -mWorldLimit ), static_cast<float>( mCenterZ2D ), 0.0f ),
					DirectX::XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f )
				);
			}

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::FrontXY>() const
			{
				return DirectX::XMMatrixLookAtRH(
					DirectX::XMVectorSet( static_cast<float>( -mCenterX2D ), static_cast<float>( mCenterY2D ), static_cast<float>( mWorldLimit ), 0.0f ),
					DirectX::XMVectorSet( static_cast<float>( -mCenterX2D ), static_cast<float>( mCenterY2D ), static_cast<float>( -mWorldLimit ), 0.0f ),
					DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f )
				);
			}

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::SideYZ>() const
			{
				return DirectX::XMMatrixLookAtRH(
					DirectX::XMVectorSet( static_cast<float>( mWorldLimit ), static_cast<float>( mCenterY2D ), static_cast<float>( mCenterZ2D ), 0.0f ),
					DirectX::XMVectorSet( static_cast<float>( -mWorldLimit ), static_cast<float>( mCenterY2D ), static_cast<float>( mCenterZ2D ), 0.0f ),
					DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f )
				);
			}

			template<EViewportType T>
			DirectX::XMMATRIX XM_CALLCONV GetProjMatrix()
			{
				ViewportElement *viewport = GetViewport<T>();
				return DirectX::XMMatrixOrthographicRH(
					static_cast<float>( viewport->GetWidth() * mZoomScale2D ),
					static_cast<float>( viewport->GetHeight() * mZoomScale2D ),
					1.0f,
					static_cast<float>( 2 * mWorldLimit )
				);
			}
		};
	}
}