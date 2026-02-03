#pragma once

// Cyclone includes
#include "Cyclone/UI/ViewportManager.hpp"

namespace Cyclone
{
	namespace Core {
		class Level;
	}

	namespace UI
	{
		class MainUI
		{
		private:
			static constexpr float kToolbarHeight = 35.0f;
			static constexpr float kSidebarWidth = 64.0f;
			static constexpr float kOutlinerWidth = 256.0f;

		public:
			MainUI() noexcept;
			~MainUI() = default;

			MainUI( MainUI && ) = default;
			MainUI &operator= ( MainUI && ) = default;

			MainUI( MainUI const & ) = delete;
			MainUI &operator= ( MainUI const & ) = delete;

			void Initialize( ID3D11Device3 *inDevice );

			void Update( float inDeltaTime, Cyclone::Core::Level *inLevel );
			void Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::Level *inLevel );

			bool IsVerticalSyncEnabled() const noexcept { return mVerticalSyncEnabled; }

		protected:
			bool mVerticalSyncEnabled;

			std::unique_ptr<Cyclone::UI::ViewportManager> mViewportManager;
		};
	}
}