#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/UI/ViewportElementPerspective.hpp"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"
#include "Cyclone/UI/ViewportType.hpp"
#include "Cyclone/UI/ViewportContext.hpp"

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
		//static constexpr double kSubGridLevels[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 5.0, 10.0 };
		//static constexpr const char *kSubGridLevelText[] = { "1cm", "5cm", "10cm", "25cm", "50cm", "1m", "5m", "10m" };
		//static double sZoomLevelToScale( int inLevel ) { return std::pow( 10.0, static_cast<double>( inLevel ) / 20.0 - 1.0 ); }

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

	protected:
		std::unique_ptr<ViewportElementPerspective> mViewportPerspective;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::TopXZ>> mViewportTop;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::FrontXY>> mViewportFront;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::SideYZ>> mViewportSide;

		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> mWireframeGridBatch;
		std::unique_ptr<DirectX::BasicEffect>	  mWireframeGridEffect;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mWireframeGridInputLayout;
		std::unique_ptr<DirectX::CommonStates>	  mCommonStates;

		bool   mShouldAutosize = false;

		ViewportGridContext			mGridContext;
		ViewportPerspectiveContext	mPerspectiveContext;
		ViewportOrthographicContext	mOrthographicContext;

		float mMinGridSize = 5;

		void UpdatePerspective( float inDeltaTime );

		template<EViewportType T> // Implemented in ViewportManager.cpp
		void UpdateWireframe( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface );

	private:

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

	};
}