#pragma once

// Cyclone includes
#include "Cyclone/Math/Vector.hpp"
#include "Cyclone/UI/ViewportElementPerspective.hpp"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"
#include "Cyclone/UI/ViewportType.hpp"
#include "Cyclone/UI/ViewportContext.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class ViewportManager
	{
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
		void Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface );

	protected:
		std::unique_ptr<ViewportElementPerspective> mViewportPerspective;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::TopXZ>> mViewportTop;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::FrontXY>> mViewportFront;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::SideYZ>> mViewportSide;

		bool   mShouldAutosize = false;

		ViewportGridContext			mGridContext;
		ViewportPerspectiveContext	mPerspectiveContext;
		ViewportOrthographicContext	mOrthographicContext;
	};
}