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

			template<size_t Axis>
			double GetCenter2D()
			{
				switch ( Axis ) {
					case 0: return mCenterX2D;
					case 1: return mCenterY2D;
					case 2: return mCenterZ2D;
					default: __assume( false );
				}
			}

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

			//template<EViewportType T>
			//void UpdateWireframe()
			//{
			//	ImVec2 viewSize = ImGui::GetWindowSize();
			//	ImVec2 viewOrigin = ImGui::GetCursorScreenPos();
			//
			//	ImGui::SetCursorPos( { 0, 0 } );
			//	ImGui::Image( mViewportTop->GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );
			//}

		private:
			inline void ComputeMinMax( double inDim, double inCenter2D, double &outMin, double &outMax )
			{
				outMin = std::max( -mWorldLimit, inDim * -mZoomScale2D / 2 + inCenter2D );
				outMax = std::min( mWorldLimit, inDim * mZoomScale2D / 2 + inCenter2D );
			}

			template<EViewportType T>
			void GetMinMaxUV( double &outMinU, double &outMaxU, double &outMinV, double &outMaxV )
			{
				constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
				constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;
				ViewportElement *viewport = GetViewport<T>();
				ComputeMinMax( static_cast<double>( viewport->GetWidth() ), GetCenter2D<AxisU>(), outMinU, outMaxU);
				ComputeMinMax( static_cast<double>( viewport->GetHeight() ), GetCenter2D<AxisV>(), outMinV, outMaxV);
			}

			template<size_t SwizzleFixed, size_t SwizzleLine>
			void DrawLineLoop( double inFixedMin, double inFixedMax, double inLineMin, double inLineMax, double inStep, DirectX::FXMVECTOR inColor )
			{
				double dFixedMin[3] = { -mCenterX2D, -mCenterY2D, -mCenterZ2D };
				double dFixedMax[3] = { -mCenterX2D, -mCenterY2D, -mCenterZ2D };
				
				dFixedMin[SwizzleFixed] += inFixedMin;
				dFixedMax[SwizzleFixed] += inFixedMax;

				for ( double line = std::round( inLineMin / inStep ) * inStep; line <= inLineMax; line += inStep ) {
					double dVarMin[3] = { dFixedMin[0], dFixedMin[1], dFixedMin[2] };
					double dVarMax[3] = { dFixedMax[0], dFixedMax[1], dFixedMax[2] };

					dVarMin[SwizzleLine] += line;
					dVarMax[SwizzleLine] += line;

					mWireframeBatch->DrawLine(
						{ DirectX::XMVectorSet( static_cast<float>( dVarMin[0] ), static_cast<float>( dVarMin[1] ), static_cast<float>( dVarMin[2] ), 0.0f ), inColor },
						{ DirectX::XMVectorSet( static_cast<float>( dVarMax[0] ), static_cast<float>( dVarMax[1] ), static_cast<float>( dVarMax[2] ), 0.0f ), inColor }
					);
				}
			}

			template<size_t Axis>
			void DrawAxisLine( double inMin, double inMax )
			{
				const DirectX::XMVECTOR colors[3] = { DirectX::Colors::DarkRed, DirectX::Colors::Green, DirectX::Colors::DarkBlue };

				double dRebaseMin[3] = { -mCenterX2D, -mCenterY2D, -mCenterZ2D };
				double dRebaseMax[3] = { -mCenterX2D, -mCenterY2D, -mCenterZ2D };

				dRebaseMin[Axis] += inMin;
				dRebaseMax[Axis] += inMax;

				mWireframeBatch->DrawLine(
					{ DirectX::XMVectorSet( static_cast<float>( dRebaseMin[0] ), static_cast<float>( dRebaseMin[1] ), static_cast<float>( dRebaseMin[2] ), 0.0f ), colors[Axis] },
					{ DirectX::XMVectorSet( static_cast<float>( dRebaseMax[0] ), static_cast<float>( dRebaseMax[1] ), static_cast<float>( dRebaseMax[2] ), 0.0f ), colors[Axis] }
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
				return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( 0.0f, static_cast<float>( 2 * mWorldLimit ), 0.0f, 0.0f ), -DirectX::g_XMIdentityR1, DirectX::g_XMIdentityR2 );
			}

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::FrontXY>() const
			{
				return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( 0.0f, 0.0f, static_cast<float>( -2 * mWorldLimit ), 0.0f ), DirectX::g_XMIdentityR2, DirectX::g_XMIdentityR1 );
			}

			template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::SideYZ>() const
			{
				return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( static_cast<float>( 2 * mWorldLimit ), 0.0f, 0.0f, 0.0f ), -DirectX::g_XMIdentityR0, DirectX::g_XMIdentityR1 );
			}

			template<EViewportType T>
			DirectX::XMMATRIX XM_CALLCONV GetProjMatrix()
			{
				ViewportElement *viewport = GetViewport<T>();
				return DirectX::XMMatrixOrthographicRH( static_cast<float>( viewport->GetWidth() * mZoomScale2D ), static_cast<float>( viewport->GetHeight() * mZoomScale2D ), 1.0f, static_cast<float>( 4 * mWorldLimit ) );
			}
		};
	}
}