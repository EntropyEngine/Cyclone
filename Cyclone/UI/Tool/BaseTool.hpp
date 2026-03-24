#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone UI
#include "Cyclone/UI/ViewportData.hpp"
#include "Cyclone/UI/ViewportType.hpp"

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI::Tool
{
	enum class EDrawMode
	{
		Always,			///< Drawing is always enabled
		WhenEnabled,	///< Drawing enabled as long as tool is enabled (and not overridden by another tool)
		WhenSelected,	///< Only draw while tool is selected
		WhenActive		///< Only draw while tool is updating
	};

	class BaseTool : public Cyclone::Util::NonCopyable
	{
	public:
		using DrawType = DirectX::PrimitiveBatch<DirectX::VertexPositionColor>;

		virtual ~BaseTool() = default;

		virtual const char *GetDebugName() const = 0;

		virtual void		OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) = 0;
		virtual void		OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) = 0;
		virtual void		OnRender( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch ) = 0;

		// TODO
		// TODO: MUST ADD "DISABLE NAVIGATION WHILE UPDATING" FLAG!!!
		// TODO

	public:
		BaseTool *			mTiedTool;
		bool				mIsSelected;	///< Is the tool currently selected
		bool				mIsActive;		///< Is the tool currently updating
		bool				mIsDrawing;		///< Is the tool currently outputting draw calls
	};
}