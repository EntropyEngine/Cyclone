#pragma once

// Cyclone includes
#include "Cyclone/UI/ViewportElement.hpp"

namespace Cyclone
{
	namespace UI
	{
		class MainUI
		{
		public:
			MainUI() noexcept;
			~MainUI() = default;

			void Initialize( ID3D11Device3 *inDevice );

			void Update( float inDeltaTime );
			void Render( ID3D11DeviceContext3 *inDeviceContext );

			bool IsVerticalSyncEnabled() const noexcept { return mVerticalSyncEnabled; }

		protected:
			bool mVerticalSyncEnabled;

			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportPerspective;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportTop;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportFront;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportSide;
		};
	}
}