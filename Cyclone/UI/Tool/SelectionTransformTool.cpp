#include "pch.h"
#include "Cyclone/UI/Tool/SelectionTransformTool.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

using Cyclone::Math::Vector4D;

template<Cyclone::UI::EViewportType T>
inline void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext, ImDrawList *inDrawList, const ImVec2 &inViewOrigin, const ImVec2 &inSelectedBoxMin, const ImVec2 &inSelectedBoxMax )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	ImGuiIO &io = ImGui::GetIO();

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &transformContext = inLevelInterface->GetSelectionTransformCtx();

	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	if ( !selectedEntities.empty() ) {

		inDrawList->AddRect( inSelectedBoxMin, inSelectedBoxMax, IM_COL32( 255, 0, 0, 255 ), 0, 0, 2 );

		for ( float x = inSelectedBoxMin.x; x < inSelectedBoxMax.x - 8; x += 16 ) {
			inDrawList->AddLine( { x, inSelectedBoxMin.y }, { x + 8, inSelectedBoxMin.y }, IM_COL32( 255, 255, 0, 255 ), 2 );
			inDrawList->AddLine( { x - 1, inSelectedBoxMax.y - 1 }, { x + 7, inSelectedBoxMax.y - 1 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}

		for ( float y = inSelectedBoxMin.y; y < inSelectedBoxMax.y - 8; y += 16 ) {
			inDrawList->AddLine( { inSelectedBoxMin.x, y }, { inSelectedBoxMin.x, y + 8 }, IM_COL32( 255, 255, 0, 255 ), 2 );
			inDrawList->AddLine( { inSelectedBoxMax.x - 1, y - 1 }, { inSelectedBoxMax.x - 1, y + 7 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}

		ImGui::SetCursorPos( { inSelectedBoxMin.x - inViewOrigin.x, inSelectedBoxMin.y - inViewOrigin.y } );
		ImGui::InvisibleButton( "Selection", { inSelectedBoxMax.x - inSelectedBoxMin.x, inSelectedBoxMax.y - inSelectedBoxMin.y }, ImGuiButtonFlags_MouseButtonLeft );
		const bool isSelectionHovered = ImGui::IsItemHovered();
		const bool isSelectionActive = ImGui::IsItemActive();
		const bool isLongClick = io.MouseDownDuration[0] > io.MouseDoubleClickTime;
		const bool isDragging = ImGui::IsMouseDragging( ImGuiMouseButton_Left );

		entt::registry &registry = inLevelInterface->GetRegistry();
		if ( isSelectionActive && ( isLongClick || isDragging ) ) {

			ImVec2 selectionMouseDrag = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );

			Cyclone::Core::Component::Position currentPosition = registry.get<Cyclone::Core::Component::Position>( selectedEntity );

			if ( !transformContext.IsActiveEntity( selectedEntity ) ) {
				assert( transformContext.GetActiveEntity() == entt::null );
				transformContext.SetActiveEntity( selectedEntity, currentPosition );
			}

			Cyclone::Core::Component::Position startPosition{ transformContext.GetInitialPosition() };
			Cyclone::Core::Component::Position positionDelta{ startPosition - currentPosition };

			if ( inGridContext.mSnapType != ViewportGridContext::ESnapType::None ) {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( std::round( -selectionMouseDrag.x * inOrthographicContext.mZoomScale2D / inGridContext.mGridSize ) * inGridContext.mGridSize );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( std::round( -selectionMouseDrag.y * inOrthographicContext.mZoomScale2D / inGridContext.mGridSize ) * inGridContext.mGridSize );

				if ( inGridContext.mSnapType == ViewportGridContext::ESnapType::ToGrid ) {
					positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( std::round( startPosition.Get<AxisU>() / inGridContext.mGridSize ) * inGridContext.mGridSize - startPosition.Get<AxisU>() );
					positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( std::round( startPosition.Get<AxisV>() / inGridContext.mGridSize ) * inGridContext.mGridSize - startPosition.Get<AxisV>() );
				}
			}
			else {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( -selectionMouseDrag.x * inOrthographicContext.mZoomScale2D );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( -selectionMouseDrag.y * inOrthographicContext.mZoomScale2D );
			}

			for ( const entt::entity entity : selectedEntities ) {
				registry.patch<Cyclone::Core::Component::Position>( entity, [positionDelta]( Cyclone::Core::Component::Position &inPosition ) { inPosition += positionDelta; } );
			}
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
			transformContext.Deactivate();
		}
	}
}

template void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate<Cyclone::UI::EViewportType::TopXZ>( Cyclone::Core::LevelInterface *, const ViewportGridContext &, const ViewportOrthographicContext &, ImDrawList *, const ImVec2 &, const ImVec2 &, const ImVec2 & );
template void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate<Cyclone::UI::EViewportType::FrontXY>( Cyclone::Core::LevelInterface *, const ViewportGridContext &, const ViewportOrthographicContext &, ImDrawList *, const ImVec2 &, const ImVec2 &, const ImVec2 & );
template void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate<Cyclone::UI::EViewportType::SideYZ>( Cyclone::Core::LevelInterface *, const ViewportGridContext &, const ViewportOrthographicContext &, ImDrawList *, const ImVec2 &, const ImVec2 &, const ImVec2 & );