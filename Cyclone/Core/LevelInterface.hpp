#pragma once

#include "Cyclone/Core/Level.hpp"

namespace Cyclone::Core
{
	class LevelInterface
	{
	public:
		LevelInterface();

		void					Initialize();

		void					SetDevice( ID3D11Device3 *inDevice );
		void					ReleaseResources();

		const ID3D11Device3 *	GetDevice() const					{ return mDevice.Get(); }
		ID3D11Device3 *			GetDevice()							{ return mDevice.Get(); }

		const Level *			GetLevel() const					{ return mLevel.get(); }
		Level *					GetLevel()							{ return mLevel.get(); }

		const entt::registry &	GetRegistry() const					{ return mLevel->GetRegistry(); }
		entt::registry &		GetRegistry()						{ return mLevel->GetRegistry(); }

		entt::entity			GetSelectedEntity() const			{ return mSelectedEntity; }
		const std::set<entt::entity> & GetSelectedEntities() const	{ return mSelectedEntities; }

		// TODO: refactor to external selection object
		void					SetSelectedEntity( entt::entity inEntity );
		void					AddSelectedEntity( entt::entity inEntity );
		void					DeselectEntity( entt::entity inEntity );
		void					ClearSelection();

	protected:
		Microsoft::WRL::ComPtr<ID3D11Device3> mDevice;

		std::unique_ptr<Level>	mLevel;

		// TODO: refactor to external selection object
		entt::entity			mSelectedEntity;
		std::set<entt::entity>	mSelectedEntities;
	};
}