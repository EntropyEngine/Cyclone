#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone UI includes
#include "Cyclone/UI/ViewportElementPerspective.hpp"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"
#include "Cyclone/UI/ViewportType.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class ViewportManager : public Cyclone::Util::NonCopyable
	{
	public:
		static constexpr float kMinViewportSize = 2.0f;

		ViewportManager();

		ViewportManager( ViewportManager && ) = default;
		ViewportManager &operator= ( ViewportManager && ) = default;

		void SetDevice( ID3D11Device3 *inDevice );

		void MenuBarUpdate();
		void Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface );
		void Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface );

	protected:
		std::unique_ptr<ViewportElementPerspective> mViewportPerspective;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::TopXZ>> mViewportTop;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::FrontXY>> mViewportFront;
		std::unique_ptr<ViewportElementOrthographic<EViewportType::SideYZ>> mViewportSide;

		bool   mShouldAutosize = true;
	};
}