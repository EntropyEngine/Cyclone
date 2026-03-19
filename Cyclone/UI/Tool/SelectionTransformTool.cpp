#include "pch.h"
#include "Cyclone/UI/Tool/SelectionTransformTool.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// ImGui Includes
#include <imgui_internal.h>

using Cyclone::Math::Vector4D;

void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	switch ( inType ) {
		case EViewportType::TopXZ: OnUpdate<EViewportType::TopXZ>( inLevelInterface, inViewportData ); break;
		case EViewportType::FrontXY: OnUpdate<EViewportType::FrontXY>( inLevelInterface, inViewportData ); break;
		case EViewportType::SideYZ: OnUpdate<EViewportType::SideYZ>( inLevelInterface, inViewportData ); break;
	}
}

void Cyclone::UI::Tool::SelectionTransformTool::OnDraw( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	switch ( inType ) {
		case EViewportType::TopXZ: OnDraw<EViewportType::TopXZ>( inLevelInterface, inViewportData ); break;
		case EViewportType::FrontXY: OnDraw<EViewportType::FrontXY>( inLevelInterface, inViewportData ); break;
		case EViewportType::SideYZ: OnDraw<EViewportType::SideYZ>( inLevelInterface, inViewportData ); break;
	}
}

template<Cyclone::UI::EViewportType T>
inline void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	ImGuiIO &io = ImGui::GetIO();

	auto &entityManager = inLevelInterface->GetEntityManager();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &transformContext = inLevelInterface->GetSelectionTransformCtx();

	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();
	entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	const double invZoom = 1.0 / orthographicContext.mZoomScale2D;
	const float offsetX = inViewportData.mViewSize.x / 2.0f + inViewportData.mViewOrigin.x;
	const float offsetY = inViewportData.mViewSize.y / 2.0f + inViewportData.mViewOrigin.y;

	if ( !selectedEntities.empty() ) {

		Vector4D selectionBoxMinRebased = transformContext.GetSelectionMin() - orthographicContext.mCenter2D;
		Vector4D selectionBoxMaxRebased = transformContext.GetSelectionMax() - orthographicContext.mCenter2D;

		ImVec2 selectedBoxMax;
		selectedBoxMax.x = offsetX - static_cast<float>( selectionBoxMinRebased.Get<AxisU>() ) * invZoom;
		selectedBoxMax.y = offsetY - static_cast<float>( selectionBoxMinRebased.Get<AxisV>() ) * invZoom;

		ImVec2 selectedBoxMin;
		selectedBoxMin.x = offsetX - static_cast<float>( selectionBoxMaxRebased.Get<AxisU>() ) * invZoom;
		selectedBoxMin.y = offsetY - static_cast<float>( selectionBoxMaxRebased.Get<AxisV>() ) * invZoom;

		ImGui::SetCursorPos( { selectedBoxMin.x - inViewportData.mViewOrigin.x, selectedBoxMin.y - inViewportData.mViewOrigin.y } );
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton( "Selection", { selectedBoxMax.x - selectedBoxMin.x, selectedBoxMax.y - selectedBoxMin.y }, ImGuiButtonFlags_MouseButtonLeft );
		const bool isLongClick = io.MouseDownDuration[0] > io.MouseDoubleClickTime;
		const bool isSelectionHovered = ImGui::IsItemHovered();
		const bool isSelectionActive = ImGui::IsItemActive();
		const bool isDragging = ImGui::IsMouseDragging( ImGuiMouseButton_Left );

		entt::registry &registry = inLevelInterface->GetRegistry();
		if ( isSelectionActive && ( isLongClick || isDragging ) ) {

			ImVec2 selectionMouseDrag = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );

			auto currentPosition = registry.get<Cyclone::Core::Component::Position>( selectedEntity ).mValue;

			if ( !transformContext.IsActiveEntity( selectedEntity ) ) {
				assert( transformContext.GetActiveEntity() == entt::null );

				if ( ImGui::IsKeyDown( ImGuiMod_Shift ) ) {
					entityManager.BeginCloneAction( registry );
					selectedEntity = selectionContext.GetSelectedEntity();
					currentPosition = registry.get<Cyclone::Core::Component::Position>( selectedEntity ).mValue;
					transformContext.SetActiveEntity( selectedEntity, currentPosition );
				}
				else {
					entityManager.BeginAction();
					transformContext.SetActiveEntity( selectedEntity, currentPosition );
				}
			}

			const Vector4D startPosition = transformContext.GetInitialPosition();
			Vector4D positionDelta = startPosition - currentPosition;

			double dragU = -selectionMouseDrag.x * orthographicContext.mZoomScale2D;
			double dragV = -selectionMouseDrag.y * orthographicContext.mZoomScale2D;

			if ( gridContext.mSnapType == Cyclone::Core::Editor::GridContext::ESnapType::ToGrid ) {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( std::round( ( dragU + startPosition.Get<AxisU>() ) / gridContext.mGridSize ) * gridContext.mGridSize - startPosition.Get<AxisU>() );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( std::round( ( dragV + startPosition.Get<AxisV>() ) / gridContext.mGridSize ) * gridContext.mGridSize - startPosition.Get<AxisV>() );
			}
			else if ( gridContext.mSnapType == Cyclone::Core::Editor::GridContext::ESnapType::ByGrid ) {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( std::round( dragU / gridContext.mGridSize ) * gridContext.mGridSize );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( std::round( dragV / gridContext.mGridSize ) * gridContext.mGridSize );
			}
			else {
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisU>( dragU );
				positionDelta += Vector4D::sZeroSetValueByIndex<AxisV>( dragV );
			}

			for ( const entt::entity entity : selectedEntities ) {
				registry.patch<Cyclone::Core::Component::Position>( entity, [positionDelta]( Cyclone::Core::Component::Position &inPosition ) { inPosition.mValue += positionDelta; } );
			}
			transformContext.UpdateOnDrag( positionDelta );
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && transformContext.GetActiveEntity() != entt::null ) {
			for ( const entt::entity entity : selectedEntities ) {
				entityManager.UpdateEntity( entity, registry );
			}
			entityManager.EndAction( registry );

			transformContext.Deactivate();
		}
		else if ( isSelectionActive ) {
			// If selection active, but long click checks failed, revert key owner to none for passthrough
			ImGui::SetKeyOwner( ImGuiKey_MouseLeft, ImGuiKeyOwner_NoOwner );
		}
	}
}

template<Cyclone::UI::EViewportType T>
inline void Cyclone::UI::Tool::SelectionTransformTool::OnDraw( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &transformContext = inLevelInterface->GetSelectionTransformCtx();

	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();

	const double invZoom = 1.0 / orthographicContext.mZoomScale2D;
	const float offsetX = inViewportData.mViewSize.x / 2.0f + inViewportData.mViewOrigin.x;
	const float offsetY = inViewportData.mViewSize.y / 2.0f + inViewportData.mViewOrigin.y;

	ImDrawList *drawList = inViewportData.mDrawList;

	if ( !selectedEntities.empty() ) {
		Vector4D selectionBoxMinRebased = transformContext.GetSelectionMin() - orthographicContext.mCenter2D;
		Vector4D selectionBoxMaxRebased = transformContext.GetSelectionMax() - orthographicContext.mCenter2D;

		ImVec2 selectedBoxMax;
		selectedBoxMax.x = offsetX - static_cast<float>( selectionBoxMinRebased.Get<AxisU>() ) * invZoom;
		selectedBoxMax.y = offsetY - static_cast<float>( selectionBoxMinRebased.Get<AxisV>() ) * invZoom;

		ImVec2 selectedBoxMin;
		selectedBoxMin.x = offsetX - static_cast<float>( selectionBoxMaxRebased.Get<AxisU>() ) * invZoom;
		selectedBoxMin.y = offsetY - static_cast<float>( selectionBoxMaxRebased.Get<AxisV>() ) * invZoom;

		if ( selectedBoxMin.x > selectedBoxMax.x ) return;
		if ( selectedBoxMin.y > selectedBoxMax.y ) return;

		drawList->AddRect( selectedBoxMin, selectedBoxMax, IM_COL32( 255, 0, 0, 255 ), 0, 0, 2 );

		for ( float x = selectedBoxMin.x; x < selectedBoxMax.x - 8; x += 16 ) {
			drawList->AddLine( { x, selectedBoxMin.y }, { x + 8, selectedBoxMin.y }, IM_COL32( 255, 255, 0, 255 ), 2 );
			drawList->AddLine( { x - 1, selectedBoxMax.y - 1 }, { x + 7, selectedBoxMax.y - 1 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}

		for ( float y = selectedBoxMin.y; y < selectedBoxMax.y - 8; y += 16 ) {
			drawList->AddLine( { selectedBoxMin.x, y }, { selectedBoxMin.x, y + 8 }, IM_COL32( 255, 255, 0, 255 ), 2 );
			drawList->AddLine( { selectedBoxMax.x - 1, y - 1 }, { selectedBoxMax.x - 1, y + 7 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}
	}
}