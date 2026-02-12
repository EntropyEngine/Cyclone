#pragma once

#include "Cyclone/UI/ViewportElement.hpp"
#include "Cyclone/UI/ViewportType.hpp"
#include "Cyclone/UI/ViewportContext.hpp"

struct ImVec2;
struct ImDrawList;

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	template<EViewportType T>
	class ViewportElementOrthographic : public ViewportElement, public ViewportTypeTraits<T>
	{
		static constexpr float kAccelerateToSnap = 10.0f;

		static constexpr float kTransformHandleSize = 8.0f;
		static constexpr float kPositionHandleSize = 4.0f;
		static constexpr float kInformationVirtualSize = 8.0f; // Pretend the position handle was this size, if the handle is wider than the bounding box hide information

		static constexpr float kMinGridSize = 5.0f;

	public:
		ViewportElementOrthographic( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor ) : ViewportElement( inBackBufferFormat, inDepthBufferFormat, inClearColor ) {}

		void Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, ViewportGridContext &inGridContext, ViewportOrthographicContext &inOrthographicContext );
		void Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext );

	protected:
		/// @note mutates global ImGui state
		void DrawEntities( const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportOrthographicContext &inOrthographicContext, ImDrawList* drawList, const ImVec2 &inViewOrigin, const ImVec2 &inViewSize, ImVec2 &outSelectedBoxMin, ImVec2 &outSelectedBoxMax ) const;
		
		void TransformSelection( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext, ImDrawList* drawList, const ImVec2 &inViewOrigin, const ImVec2 &inSelectedBoxMin, const ImVec2 &inSelectedBoxMax );

	private:
		void XM_CALLCONV GetMinMaxUV( Cyclone::Math::XLVector inCenter2D, double inWorldLimit, double inZoomScale2D, double &outMinU, double &outMaxU, double &outMinV, double &outMaxV ) const
		{
			double mCenterU = inCenter2D.Get<ViewportElementOrthographic::AxisU>();
			double mCenterV = inCenter2D.Get<ViewportElementOrthographic::AxisV>();

			outMinU = std::max( -inWorldLimit, mWidth * -inZoomScale2D / 2 + mCenterU );
			outMaxU = std::min( inWorldLimit, mWidth * inZoomScale2D / 2 + mCenterU );

			outMinV = std::max( -inWorldLimit, mHeight * -inZoomScale2D / 2 + mCenterV );
			outMaxV = std::min( inWorldLimit, mHeight * inZoomScale2D / 2 + mCenterV );
		}

		template<size_t SwizzleFixed, size_t SwizzleLine>
		void XM_CALLCONV DrawLineLoop( Cyclone::Math::XLVector inCenter2D, double inFixedMin, double inFixedMax, double inLineMin, double inLineMax, double inStep, DirectX::FXMVECTOR inColor ) const
		{
			Cyclone::Math::XLVector negativeCenter = -inCenter2D;
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
		void XM_CALLCONV DrawAxisLine( Cyclone::Math::XLVector inCenter2D, double inMin, double inMax ) const
		{
			const DirectX::XMVECTOR colors[3] = { DirectX::Colors::DarkRed, DirectX::Colors::Green, DirectX::Colors::DarkBlue };

			Cyclone::Math::XLVector negativeCenter = -inCenter2D;
			Cyclone::Math::XLVector rebasedMin = negativeCenter + Cyclone::Math::XLVector::sZeroSetValueByIndex<Axis>( inMin );
			Cyclone::Math::XLVector rebasedMax = negativeCenter + Cyclone::Math::XLVector::sZeroSetValueByIndex<Axis>( inMax );

			mWireframeGridBatch->DrawLine(
				{ rebasedMin.ToXMVECTOR(), colors[Axis] },
				{ rebasedMax.ToXMVECTOR(), colors[Axis] }
			);
		}
	};
}