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
			static constexpr double kSubGridLevels[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0 };
			static constexpr const char *kSubGridLevelText[] = { "1cm", "5cm", "10cm", "25cm", "50cm", "1m", "2.5m", "5m", "10m" };
			static double sZoomLevelToScale( int inLevel ) { return std::pow( 10.0, static_cast<double>( inLevel ) / 20.0 - 1.0 ); }

		public:
			ViewportManager( ID3D11Device3 *inDevice );

			ViewportManager( ViewportManager && ) = default;
			ViewportManager &operator= ( ViewportManager && ) = default;

			ViewportManager( ViewportManager const & ) = delete;
			ViewportManager &operator= ( ViewportManager const & ) = delete;

			void ToolbarUpdate();
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

			int    mZoomLevel = 0;
			double mZoomScale2D = sZoomLevelToScale( mZoomLevel ); // Pixels to meters

			double mWorldLimit = 10000.0; // World +/- limit in meters

			int	   mSubGridSizeIndex = 5;
			double mSubGridSize = kSubGridLevels[mSubGridSizeIndex]; // Grid spacing in meters

			double mMinGridSize = 8.0; // Min subgrid view size in pixels

			double mCenterX2D = 0.0;
			double mCenterY2D = 0.0;
			double mCenterZ2D = 0.0;

			template<EViewportType T>
			void RenderWireFrame( ID3D11DeviceContext3 *inDeviceContext ); // Implemented in ViewportManager.cpp

		private:
			inline void ComputeMinMax( double inDim, double inCenter2D, double &outMin, double &outMax )
			{
				outMin = std::max( -mWorldLimit, inDim * -mZoomScale2D / 2 + inCenter2D );
				outMax = std::min( mWorldLimit, inDim * mZoomScale2D / 2 + inCenter2D );
			}

			template<EViewportType T>
			void GetMinMaxUV( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV );

			template<> void GetMinMaxUV<EViewportType::TopXZ>( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				ViewportElement *viewport = GetViewport<EViewportType::TopXZ>();
				ComputeMinMax( static_cast<double>( viewport->GetWidth() ), mCenterX2D, outMinU, outMaxU );
				ComputeMinMax( static_cast<double>( viewport->GetHeight() ), mCenterZ2D, outMinV, outMaxV );
			}

			template<> void GetMinMaxUV<EViewportType::FrontXY>( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				ViewportElement *viewport = GetViewport<EViewportType::FrontXY>();
				ComputeMinMax( static_cast<double>( viewport->GetWidth() ), -mCenterX2D, outMinU, outMaxU );
				ComputeMinMax( static_cast<double>( viewport->GetHeight() ), mCenterY2D, outMinV, outMaxV );
			}

			template<> void GetMinMaxUV<EViewportType::SideYZ>( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				ViewportElement *viewport = GetViewport<EViewportType::SideYZ>();
				ComputeMinMax( static_cast<double>( viewport->GetWidth() ), mCenterZ2D, outMinU, outMaxU );
				ComputeMinMax( static_cast<double>( viewport->GetHeight() ), mCenterY2D, outMinV, outMaxV );
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
			void DrawAxisLine( double inMin, double inMax )
			{
				const DirectX::XMVECTOR colors[3] = { DirectX::Colors::DarkRed, DirectX::Colors::Green, DirectX::Colors::DarkBlue };
				mWireframeBatch->DrawLine(
					{ DirectX::XMVectorSetByIndex( DirectX::g_XMZero, static_cast<float>( inMin ), Axis ), colors[Axis] },
					{ DirectX::XMVectorSetByIndex( DirectX::g_XMZero, static_cast<float>( inMax ), Axis ), colors[Axis] }
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
				return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( static_cast<float>( mCenterX2D ), static_cast<float>( mWorldLimit ), static_cast<float>( mCenterZ2D ), 0.0f ), -DirectX::g_XMIdentityR1, DirectX::g_XMIdentityR2 );
			}

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::FrontXY>() const
			{
				return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( static_cast<float>( -mCenterX2D ), static_cast<float>( mCenterY2D ), static_cast<float>( mWorldLimit ), 0.0f ), -DirectX::g_XMIdentityR2, DirectX::g_XMIdentityR1 );
			}

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::SideYZ>() const
			{
				return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( static_cast<float>( mWorldLimit ), static_cast<float>( mCenterY2D ), static_cast<float>( mCenterZ2D ), 0.0f ), -DirectX::g_XMIdentityR0, DirectX::g_XMIdentityR1 );
			}

			template<EViewportType T>
			DirectX::XMMATRIX XM_CALLCONV GetProjMatrix()
			{
				ViewportElement *viewport = GetViewport<T>();
				return DirectX::XMMatrixOrthographicRH( static_cast<float>( viewport->GetWidth() * mZoomScale2D ), static_cast<float>( viewport->GetHeight() * mZoomScale2D ), 1.0f, static_cast<float>( 2 * mWorldLimit ) );
			}
		};
	}
}