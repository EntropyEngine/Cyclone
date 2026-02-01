#pragma once

// Cyclone includes
#include "Cyclone/UI/ViewportManager.hpp"

namespace Cyclone
{
	namespace UI
	{
		class MainUI
		{
		public:
			MainUI() noexcept;
			~MainUI() = default;

			MainUI( MainUI && ) = default;
			MainUI &operator= ( MainUI && ) = default;

			MainUI( MainUI const & ) = delete;
			MainUI &operator= ( MainUI const & ) = delete;

			void Initialize( ID3D11Device3 *inDevice );

			void Update( float inDeltaTime );
			void Render( ID3D11DeviceContext3 *inDeviceContext );

			bool IsVerticalSyncEnabled() const noexcept { return mVerticalSyncEnabled; }

		protected:
			bool mVerticalSyncEnabled;

			std::unique_ptr<Cyclone::UI::ViewportManager> mViewportManager;
		};
	}
}