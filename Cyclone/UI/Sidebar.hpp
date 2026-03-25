#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone UI includes
#include "Cyclone/UI/Tool/BaseTool.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	class Sidebar : public Cyclone::Util::NonCopyable
	{
	public:
		void Init();
		void Update( Cyclone::Core::LevelInterface *inLevelInterface );
		const std::span<std::unique_ptr<Tool::BaseTool>> GetTools() { return mToolChanger;  }

	protected:
		void SelectTool( Tool::BaseTool *inTool );

		std::vector<std::unique_ptr<Tool::BaseTool>> mToolChanger;
		std::array<std::vector<Tool::BaseTool *>, static_cast<size_t>( Tool::ECategory::COUNT )> mToolCategories;
	};
}