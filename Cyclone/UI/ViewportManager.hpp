#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/UI/ViewportElement.hpp"
#include "Cyclone/UI/ViewportType.hpp"

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class ViewportManager
	{
		static constexpr double kSubGridLevels[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0 };
		static constexpr const char *kSubGridLevelText[] = { "1cm", "5cm", "10cm", "25cm", "50cm", "1m", "2.5m", "5m", "10m" };
		static double sZoomLevelToScale( int inLevel ) { return std::pow( 10.0, static_cast<double>( inLevel ) / 20.0 - 1.0 ); }

		static constexpr float kMouseSensitivity = 0.01f;
		static constexpr float kKeyboardSensitivity = 5.0f;
		static constexpr float kCameraDollySensitivity = 5.0f;
		static constexpr float kHorizontalFOV = DirectX::XM_PIDIV2;

		static constexpr float kAccelerateToSnap = 10.0f;

		static constexpr float kTransformHandleSize = 8.0f;
		static constexpr float kPositionHandleSize = 4.0f;
		static constexpr float kInformationVirtualSize = 8.0f; // Pretend the position handle was this size, if the handle is wider than the bounding box hide information

	public:
		ViewportManager();

		ViewportManager( ViewportManager && ) = default;
		ViewportManager &operator= ( ViewportManager && ) = default;

		ViewportManager( ViewportManager const & ) = delete;
		ViewportManager &operator= ( ViewportManager const & ) = delete;

		void SetDevice( ID3D11Device3 *inDevice );

		void MenuBarUpdate();
		void ToolbarUpdate();
		void Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface );

		void RenderPerspective( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface );
		void RenderTop( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface );
		void RenderFront( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface );
		void RenderSide( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface );

		template<size_t Axis>
		double GetCenter2D()
		{
			switch ( Axis ) {
				case 0: return mCenter2D.GetX();
				case 1: return mCenter2D.GetY();
				case 2: return mCenter2D.GetZ();
				default: __assume( false );
			}
		}

	protected:
		std::unique_ptr<Cyclone::UI::ViewportElement> mViewportPerspective;
		std::unique_ptr<Cyclone::UI::ViewportElement> mViewportTop;
		std::unique_ptr<Cyclone::UI::ViewportElement> mViewportFront;
		std::unique_ptr<Cyclone::UI::ViewportElement> mViewportSide;

		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> mWireframeGridBatch;
		std::unique_ptr<DirectX::BasicEffect>	  mWireframeGridEffect;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mWireframeGridInputLayout;
		std::unique_ptr<DirectX::CommonStates>	  mCommonStates;

		bool   mShouldAutosize = false;

		int    mZoomLevel = 0;
		double mZoomScale2D = sZoomLevelToScale( mZoomLevel ); // Pixels to meters

		double mWorldLimit = 10000.0; // World +/- limit in meters

		int	   mSubGridSizeIndex = 5;
		double mSubGridSize = kSubGridLevels[mSubGridSizeIndex]; // Grid spacing in meters

		double mMinGridSize = 8.0; // Min subgrid view size in pixels

		float					mCameraPitch = 0.0f;
		float					mCameraYaw = 0.0f;
		Cyclone::Math::XLVector mCenter3D = Cyclone::Math::XLVector( 3, 3, -6 );
		Cyclone::Math::XLVector mCenter2D = Cyclone::Math::XLVector::sZero();

		void UpdatePerspective( float inDeltaTime );

		template<EViewportType T> // Implemented in ViewportManager.cpp
		void UpdateWireframe( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface );

		template<EViewportType T>
		void RenderWireframe( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface ); // Implemented in ViewportManager.cpp

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
			Cyclone::Math::XLVector negativeCenter = -mCenter2D;
			Cyclone::Math::XLVector fixedMin = negativeCenter + Cyclone::Math::XLVector::sZeroSetValueByIndex<SwizzleFixed>( inFixedMin );
			Cyclone::Math::XLVector fixedMax = negativeCenter + Cyclone::Math::XLVector::sZeroSetValueByIndex<SwizzleFixed>( inFixedMax );

			for ( double line = std::round( inLineMin / inStep ) * inStep; line <= inLineMax; line += inStep ) {
				Cyclone::Math::XLVector varLine = Cyclone::Math::XLVector::sZeroSetValueByIndex<SwizzleLine>( line );
				Cyclone::Math::XLVector varMin = fixedMin + varLine;
				Cyclone::Math::XLVector varMax = fixedMax + varLine;

				mWireframeGridBatch->DrawLine(
					{ varMin.ToXMVECTOR(), inColor },
					{ varMax.ToXMVECTOR(), inColor }
				);
			}
		}

		template<size_t Axis>
		void DrawAxisLine( double inMin, double inMax )
		{
			const DirectX::XMVECTOR colors[3] = { DirectX::Colors::DarkRed, DirectX::Colors::Green, DirectX::Colors::DarkBlue };

			Cyclone::Math::XLVector negativeCenter = -mCenter2D;
			Cyclone::Math::XLVector rebasedMin = negativeCenter + Cyclone::Math::XLVector::sZeroSetValueByIndex<Axis>( inMin );
			Cyclone::Math::XLVector rebasedMax = negativeCenter + Cyclone::Math::XLVector::sZeroSetValueByIndex<Axis>( inMax );

			mWireframeGridBatch->DrawLine(
				{ rebasedMin.ToXMVECTOR(), colors[Axis] },
				{ rebasedMax.ToXMVECTOR(), colors[Axis] }
			);
		}


		template<EViewportType T>
		ViewportElement *GetViewport();

		template<> ViewportElement *GetViewport<EViewportType::Perspective>() { return mViewportPerspective.get(); }
		template<> ViewportElement *GetViewport<EViewportType::TopXZ>()		  { return mViewportTop.get(); }
		template<> ViewportElement *GetViewport<EViewportType::FrontXY>()	  { return mViewportFront.get(); }
		template<> ViewportElement *GetViewport<EViewportType::SideYZ>()	  { return mViewportSide.get(); }

		template<EViewportType T>
		const ViewportElement *GetViewport() const;

		template<> const ViewportElement *GetViewport<EViewportType::Perspective>() const { return mViewportPerspective.get(); }
		template<> const ViewportElement *GetViewport<EViewportType::TopXZ>() const		  { return mViewportTop.get(); }
		template<> const ViewportElement *GetViewport<EViewportType::FrontXY>() const	  { return mViewportFront.get(); }
		template<> const ViewportElement *GetViewport<EViewportType::SideYZ>() const	  { return mViewportSide.get(); }


		template<EViewportType T>
		DirectX::XMMATRIX XM_CALLCONV GetViewMatrix() const;

		template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<EViewportType::Perspective>() const
		{
			return DirectX::XMMatrixLookToRH( DirectX::g_XMZero, DirectX::XMVector3Transform( DirectX::g_XMIdentityR2, DirectX::XMMatrixRotationRollPitchYaw( mCameraPitch, mCameraYaw, 0.0f ) ), DirectX::g_XMIdentityR1 );
			//return DirectX::XMMatrixLookToRH( DirectX::g_XMZero, -DirectX::g_XMIdentityR1, DirectX::g_XMIdentityR2 );
		}

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
		DirectX::XMMATRIX XM_CALLCONV GetProjMatrix() const
		{
			const ViewportElement *viewport = GetViewport<T>();
			return DirectX::XMMatrixOrthographicRH( static_cast<float>( viewport->GetWidth() * mZoomScale2D ), static_cast<float>( viewport->GetHeight() * mZoomScale2D ), 1.0f, static_cast<float>( 4 * mWorldLimit ) );
		}

		template<>
		DirectX::XMMATRIX XM_CALLCONV GetProjMatrix<EViewportType::Perspective>() const
		{
			const ViewportElement *viewport = GetViewport<EViewportType::Perspective>();

			float aspectRatio = static_cast<float>( viewport->GetWidth() ) / static_cast<float>( viewport->GetHeight() );
			float fovY = 2.0f * std::atan( std::tan( kHorizontalFOV / 2 ) / aspectRatio );
			return DirectX::XMMatrixPerspectiveFovRH( fovY, aspectRatio, 0.1f, static_cast<float>( 2 * mWorldLimit ) );
		}
	};
}