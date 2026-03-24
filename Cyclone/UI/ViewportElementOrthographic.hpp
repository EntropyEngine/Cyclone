#pragma once

// Cyclone UI includes
#include "Cyclone/UI/ViewportElement.hpp"
#include "Cyclone/UI/ViewportType.hpp"
#include "Cyclone/UI/Tool/BaseTool.hpp"

// Cyclone Math
#include "Cyclone/Math/Vector.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	template<EViewportType T>
	class ViewportElementOrthographic : public ViewportElement, public ViewportTypeTraits<T>
	{
		static constexpr float kAccelerateToSnap = 10.0f;

	public:
		ViewportElementOrthographic( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor, bool inAntialiasing ) : ViewportElement( inBackBufferFormat, inDepthBufferFormat, inClearColor, inAntialiasing ) {}

		void UpdateNavigation( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface );
		void UpdateTools( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools );
		void DrawGizmos( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools );
		void Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools );

		Cyclone::Math::Vector4D XM_CALLCONV GetViewBoundingBoxExtent( double inWorldLimit, double inZoomScale2D )
		{
			double mExtentU = mViewportData.mViewSize.x * inZoomScale2D / 2;
			double mExtentV = mViewportData.mViewSize.y * inZoomScale2D / 2;
			double mExtentW = inWorldLimit;

			Cyclone::Math::Vector4D extent = Cyclone::Math::Vector4D::sZero();
			extent.Set<ViewportElementOrthographic::AxisU>( mExtentU );
			extent.Set<ViewportElementOrthographic::AxisV>( mExtentV );
			extent.Set<ViewportElementOrthographic::AxisW>( mExtentW );

			return extent;
		}

	protected:
		/// @note mutates global ImGui state
		void DrawEntities( Cyclone::Core::LevelInterface *inLevelInterface ) const;

	private:
		void XM_CALLCONV GetMinMaxUV( Cyclone::Math::Vector4D inCenter2D, double inWorldLimit, double inZoomScale2D, double &outMinU, double &outMaxU, double &outMinV, double &outMaxV ) const
		{
			double mCenterU = inCenter2D.Get<ViewportElementOrthographic::AxisU>();
			double mCenterV = inCenter2D.Get<ViewportElementOrthographic::AxisV>();

			outMinU = std::max( -inWorldLimit, mWidth * -inZoomScale2D / 2 + mCenterU );
			outMaxU = std::min( inWorldLimit, mWidth * inZoomScale2D / 2 + mCenterU );

			outMinV = std::max( -inWorldLimit, mHeight * -inZoomScale2D / 2 + mCenterV );
			outMaxV = std::min( inWorldLimit, mHeight * inZoomScale2D / 2 + mCenterV );
		}

		template<size_t SwizzleFixed, size_t SwizzleLine>
		void XM_CALLCONV DrawLineLoop( Cyclone::Math::Vector4D inCenter2D, double inFixedMin, double inFixedMax, double inLineMin, double inLineMax, double inStep, DirectX::FXMVECTOR inColor ) const
		{
			Cyclone::Math::Vector4D negativeCenter = -inCenter2D;
			Cyclone::Math::Vector4D fixedMin = negativeCenter + Cyclone::Math::Vector4D::sZeroSetValueByIndex<SwizzleFixed>( inFixedMin );
			Cyclone::Math::Vector4D fixedMax = negativeCenter + Cyclone::Math::Vector4D::sZeroSetValueByIndex<SwizzleFixed>( inFixedMax );

			for ( double line = std::round( inLineMin / inStep ) * inStep; line <= inLineMax; line += inStep ) {
				Cyclone::Math::Vector4D varLine = Cyclone::Math::Vector4D::sZeroSetValueByIndex<SwizzleLine>( line );
				Cyclone::Math::Vector4D varMin = fixedMin + varLine;
				Cyclone::Math::Vector4D varMax = fixedMax + varLine;

				mWireframeGridBatch->DrawLine(
					{ varMin.ToXMVECTOR(), inColor },
					{ varMax.ToXMVECTOR(), inColor }
				);
			}
		}

		template<size_t Axis>
		void XM_CALLCONV DrawAxisLine( Cyclone::Math::Vector4D inCenter2D, double inMin, double inMax ) const
		{
			const DirectX::XMVECTOR colors[3] = { DirectX::Colors::DarkRed, DirectX::Colors::Green, DirectX::Colors::DarkBlue };

			Cyclone::Math::Vector4D negativeCenter = -inCenter2D;
			Cyclone::Math::Vector4D rebasedMin = negativeCenter + Cyclone::Math::Vector4D::sZeroSetValueByIndex<Axis>( inMin );
			Cyclone::Math::Vector4D rebasedMax = negativeCenter + Cyclone::Math::Vector4D::sZeroSetValueByIndex<Axis>( inMax );

			mWireframeGridBatch->DrawLine(
				{ rebasedMin.ToXMVECTOR(), colors[Axis] },
				{ rebasedMax.ToXMVECTOR(), colors[Axis] }
			);
		}
	};
}