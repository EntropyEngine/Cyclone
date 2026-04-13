#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/HashMap.hpp"

namespace Cyclone::Core
{
	struct HistoryAction
	{
		entt::registry mRegistry;

		Cyclone::Util::OptionalHashPair<bool> mEntityTypeSelectable;
		Cyclone::Util::OptionalHashPair<bool> mEntityTypeVisible;

		Cyclone::Util::OptionalHashPair<bool> mEntityCategorySelectable;
		Cyclone::Util::OptionalHashPair<bool> mEntityCategoryVisible;

		std::set<entt::entity> mSelectedEntities;
		entt::entity mSelectedEntity = entt::null;
	};
}