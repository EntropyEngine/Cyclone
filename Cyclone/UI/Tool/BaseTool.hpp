#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone UI
#include "Cyclone/UI/ViewportData.hpp"
#include "Cyclone/UI/ViewportType.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	class BaseTool : public Cyclone::Util::NonCopyable
	{
	public:
		virtual ~BaseTool() = default;

		virtual void	OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) = 0;
		virtual void	OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) = 0;

		bool			IsSelected() const	{ return mIsSelected; }
		bool			IsActive() const	{ return mIsActive; }
		bool			IsDrawing() const	{ return mIsDrawing; }

	protected:
		bool			mIsSelected;	///< Is the tool currently selected
		bool			mIsActive;		///< Is the tool currently updating
		bool			mIsDrawing;		///< Is the tool currently outputting draw calls
	};
}