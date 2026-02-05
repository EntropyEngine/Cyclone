#pragma once

namespace Cyclone::Core
{
	/// @brief The Level object which holds all entities' persistant and transient data
	///
	/// Within the Level object is the entity registry which stores all entities and their components.
	/// The components can be split into two categories:
	/// - Persistant components: this is the data which will be stored in the level file and represents all entity information; i.e. the primary sources of truth.
	/// - Transient components: this is the data used to help render and manipulate entities in the editor; i.e. secondary sources of truth which act as caches.
	/// 
	/// The level class should never directly interact with the user interface except for providing a shortcut to DirectX resources for drawing, or exposing level-specific config settings that live outside the ECS.
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