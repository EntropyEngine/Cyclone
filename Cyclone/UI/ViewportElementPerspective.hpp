#pragma once

#include "Cyclone/UI/ViewportElement.hpp"
#include "Cyclone/UI/ViewportContext.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class ViewportElementPerspective : public ViewportElement
	{
		static constexpr float kMouseSensitivity = 0.01f;
		static constexpr float kKeyboardSensitivity = 5.0f;
		static constexpr float kCameraDollySensitivity = 5.0f;
		static constexpr float kHorizontalFOV = DirectX::XM_PIDIV2;

	public:
		ViewportElementPerspective( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor ) : ViewportElement( inBackBufferFormat, inDepthBufferFormat, inClearColor ) {}

		void Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, ViewportGridContext &inGridContext, ViewportPerspectiveContext &inPerspectiveContext );
		void Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportPerspectiveContext &inPerspectiveContext );
	};
}