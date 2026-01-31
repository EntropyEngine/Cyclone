#pragma once

namespace Cyclone
{
	namespace UI
	{
		class MainUI
		{
		public:
			MainUI() noexcept;
			~MainUI() = default;

			void Initialize();

			void Update( float inDeltaTime );

			bool IsVerticalSyncEnabled() const noexcept { return mVerticalSyncEnabled; }

		protected:
			bool mVerticalSyncEnabled;
		};
	}
}