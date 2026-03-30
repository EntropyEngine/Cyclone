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
	enum class ECategory
	{
		Object,
		EditPath,
		EditMesh,
		COUNT,
	};

	inline constexpr const char* kCategoryNames[] = {
		"Object",
		"Path",
		"Mesh"
	};

	enum class ESelectMode
	{
		ToggleInCategory,	///< Can be toggled on or off, will deselect tools in other categories
		SelectInCategory,	///< Can be selected in a category, will deselect tools in other categories and other non-toggleable tools in this category
		UniqueInCategory,	///< Cab be selected in a category, will deselect all other tools
	};

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
		virtual ECategory	GetCategory() const = 0;
		virtual ESelectMode GetSelectMode() const = 0;

	#pragma warning(push)
	#pragma warning(disable: 4100)
		virtual void		OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) {};
		virtual void		OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData ) {};
		virtual void		OnRender( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch ) {};
		virtual void		OnShortcut( Cyclone::Core::LevelInterface *inLevelInterface ) {}
	#pragma warning(pop)

		// TODO
		// TODO: MUST ADD "DISABLE NAVIGATION WHILE UPDATING" FLAG!!!
		// TODO

	public:
		bool				mIsSelected{ false };	///< Is the tool currently selected
		bool				mIsActive{ false };		///< Is the tool currently updating
		bool				mIsDrawing{ false };	///< Is the tool currently outputting draw calls
	};
}