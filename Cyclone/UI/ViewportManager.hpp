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

			float mGridSize = 10.0f; // Grid spacing in meters
			float mSubGridSize = 1.0f; // Grid spacing in meters

			double mCenterX2D = 0.0;
			double mCenterY2D = 0.0;
			double mCenterZ2D = 0.0;
		};
	}
}