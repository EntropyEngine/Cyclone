#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

namespace Cyclone::UI::Tool {
	class SelectionTool;
}

namespace Cyclone::Core::Tool
{
	class SelectionToolContext : public Cyclone::Util::NonCopyable
	{
	public:
		friend Cyclone::UI::Tool::SelectionTool;

		entt::entity			GetSelectedEntity() const			{ return mSelectedEntity; }
		const std::set<entt::entity> & GetSelectedEntities() const	{ return mSelectedEntities; }

		void					SetSelectedEntity( entt::entity inEntity );
		void					AddSelectedEntity( entt::entity inEntity );
		void					DeselectEntity( entt::entity inEntity );
		void					ClearSelection();

	protected:
		entt::entity			mSelectedEntity = entt::null;
		std::set<entt::entity>	mSelectedEntities;
		std::set<entt::entity>	mPreviousCandidates;
	};
}