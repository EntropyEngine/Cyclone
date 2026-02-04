#pragma once

namespace Cyclone
{
	namespace Core
	{
		class Level
		{
		public:
			Level() = default;

			void Initialize();

			void SetDevice( ID3D11Device3 *inDevice );
			void ReleaseResources();

		protected:
			entt::registry mRegistry;

			Microsoft::WRL::ComPtr<ID3D11Device3> mDevice;
		};
	}
}