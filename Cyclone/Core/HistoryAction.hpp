#pragma once

// Cyclone Utils
#include "Cyclone/Util/NonCopyable.hpp"
#include "Cyclone/Util/HashMap.hpp"

// STL
#include <optional>

namespace Cyclone::Core
{
	struct HistoryAction : public Cyclone::Util::NonCopyable
	{
		entt::registry mRegistry;

		std::optional<Cyclone::Util::HashPair<bool>> mEntityTypeSelectable;
		std::optional<Cyclone::Util::HashPair<bool>> mEntityTypeVisible;

		std::optional<Cyclone::Util::HashPair<bool>> mEntityCategorySelectable;
		std::optional<Cyclone::Util::HashPair<bool>> mEntityCategoryVisible;

		std::set<entt::entity> mSelectedEntities;
		entt::entity mSelectedEntity = entt::null;
	};
}