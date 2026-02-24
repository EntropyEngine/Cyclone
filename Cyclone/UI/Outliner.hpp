#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/EntityCategory.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class Outliner : public Cyclone::Util::NonCopyable
	{
	public:
		void Update( Cyclone::Core::LevelInterface *inLevelInterface );

	protected:
		using EntityList = std::vector<entt::entity>;
		using EntityTypeTree = std::map<Cyclone::Core::Component::EntityType, EntityList>;
		using EntityCategoryTree = std::map<Cyclone::Core::Component::EntityCategory, EntityTypeTree>;
		EntityCategoryTree mOutlinerTree;

		float mOutlinerHeight = 256.0f;
		float mSelectionHeight = 256.0f;
		float mUndoHistoryHeight = 256.0f;

		float mRemainingHeight = 0.0f;

		void RebuildTree( const Cyclone::Core::LevelInterface *inLevelInterface );
	};
}