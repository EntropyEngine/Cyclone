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
inline void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface )
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
	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	ImGui::SetCursorPos( { 0, 0 } );
	ImVec2 inViewOrigin = ImGui::GetCursorScreenPos();
	ImVec2 inViewSize = ImGui::GetWindowSize();

	ImDrawList* inDrawList = ImGui::GetWindowDrawList();
	inDrawList->Flags |= ImDrawListFlags_AntiAliasedLines;

	const double invZoom = 1.0 / orthographicContext.mZoomScale2D;
	const float offsetX = inViewSize.x / 2.0f + inViewOrigin.x;
	const float offsetY = inViewSize.y / 2.0f + inViewOrigin.y;

	ImVec2 inSelectedBoxMin;
	ImVec2 inSelectedBoxMax;

	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	for ( const entt::entity entity : selectedEntities ) {
		const auto &position = cregistry.get<Cyclone::Core::Component::Position>( entity ).mValue;
		const auto &boundingBox = cregistry.get<Cyclone::Core::Component::BoundingBox>( entity ).mValue;

		Vector4D rebasedEntityPosition = ( orthographicContext.mCenter2D - position );
		Vector4D rebasedBoundingBoxMin = rebasedEntityPosition - boundingBox.mCenter - boundingBox.mExtent;
		Vector4D rebasedBoundingBoxMax = rebasedEntityPosition - boundingBox.mCenter + boundingBox.mExtent;

		ImVec2 localBoxMin;
		localBoxMin.x = static_cast<float>( rebasedBoundingBoxMin.Get<AxisU>() * invZoom ) + offsetX;
		localBoxMin.y = static_cast<float>( rebasedBoundingBoxMin.Get<AxisV>() * invZoom ) + offsetY;

		ImVec2 localBoxMax;
		localBoxMax.x = static_cast<float>( rebasedBoundingBoxMax.Get<AxisU>() * invZoom ) + offsetX;
		localBoxMax.y = static_cast<float>( rebasedBoundingBoxMax.Get<AxisV>() * invZoom ) + offsetY;

		inSelectedBoxMin.x = std::min( inSelectedBoxMin.x, localBoxMin.x );
		inSelectedBoxMin.y = std::min( inSelectedBoxMin.y, localBoxMin.y );

		inSelectedBoxMax.x = std::max( inSelectedBoxMax.x, localBoxMax.x );
		inSelectedBoxMax.y = std::max( inSelectedBoxMax.y, localBoxMax.y );
	}

	if ( !selectedEntities.empty() ) {
		ImGui::SetCursorPos( { inSelectedBoxMin.x - inViewOrigin.x, inSelectedBoxMin.y - inViewOrigin.y } );
		ImGui::InvisibleButton( "Selection", { inSelectedBoxMax.x - inSelectedBoxMin.x, inSelectedBoxMax.y - inSelectedBoxMin.y }, ImGuiButtonFlags_MouseButtonLeft );
		const bool isSelectionHovered = ImGui::IsItemHovered();
		const bool isSelectionActive = ImGui::IsItemActive();
		const bool isLongClick = io.MouseDownDuration[0] > io.MouseDoubleClickTime;
		const bool isDragging = ImGui::IsMouseDragging( ImGuiMouseButton_Left );

		entt::registry &registry = inLevelInterface->GetRegistry();
		if ( isSelectionActive && ( isLongClick || isDragging ) ) {

			ImVec2 selectionMouseDrag = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );

			const auto &currentPosition = registry.get<Cyclone::Core::Component::Position>( selectedEntity ).mValue;

			if ( !transformContext.IsActiveEntity( selectedEntity ) ) {
				assert( transformContext.GetActiveEntity() == entt::null );

				entityManager.BeginAction();
				transformContext.SetActiveEntity( selectedEntity, currentPosition );
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
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && transformContext.GetActiveEntity() != entt::null ) {
			for ( const entt::entity entity : selectedEntities ) {
				entityManager.UpdateEntity( entity, registry );
			}
			entityManager.EndAction( registry );

			transformContext.Deactivate();
		}
	}
}

template<Cyclone::UI::EViewportType T>
inline void Cyclone::UI::Tool::SelectionTransformTool::OnDraw( Cyclone::Core::LevelInterface *inLevelInterface )
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
	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();

	ImGui::SetCursorPos( { 0, 0 } );
	ImVec2 inViewOrigin = ImGui::GetCursorScreenPos();
	ImVec2 inViewSize = ImGui::GetWindowSize();

	ImDrawList* inDrawList = ImGui::GetWindowDrawList();
	inDrawList->Flags |= ImDrawListFlags_AntiAliasedLines;

	const double invZoom = 1.0 / orthographicContext.mZoomScale2D;
	const float offsetX = inViewSize.x / 2.0f + inViewOrigin.x;
	const float offsetY = inViewSize.y / 2.0f + inViewOrigin.y;

	ImVec2 inSelectedBoxMin = { inViewOrigin.x + inViewSize.x, inViewOrigin.y + inViewSize.y };
	ImVec2 inSelectedBoxMax = inViewOrigin;

	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	for ( const entt::entity entity : selectedEntities ) {
		const auto &position = cregistry.get<Cyclone::Core::Component::Position>( entity ).mValue;
		const auto &boundingBox = cregistry.get<Cyclone::Core::Component::BoundingBox>( entity ).mValue;

		Vector4D rebasedEntityPosition = ( orthographicContext.mCenter2D - position );
		Vector4D rebasedBoundingBoxMin = rebasedEntityPosition - boundingBox.mCenter - boundingBox.mExtent;
		Vector4D rebasedBoundingBoxMax = rebasedEntityPosition - boundingBox.mCenter + boundingBox.mExtent;

		ImVec2 localBoxMin;
		localBoxMin.x = static_cast<float>( rebasedBoundingBoxMin.Get<AxisU>() * invZoom ) + offsetX;
		localBoxMin.y = static_cast<float>( rebasedBoundingBoxMin.Get<AxisV>() * invZoom ) + offsetY;

		ImVec2 localBoxMax;
		localBoxMax.x = static_cast<float>( rebasedBoundingBoxMax.Get<AxisU>() * invZoom ) + offsetX;
		localBoxMax.y = static_cast<float>( rebasedBoundingBoxMax.Get<AxisV>() * invZoom ) + offsetY;

		inSelectedBoxMin.x = std::min( inSelectedBoxMin.x, localBoxMin.x );
		inSelectedBoxMin.y = std::min( inSelectedBoxMin.y, localBoxMin.y );

		inSelectedBoxMax.x = std::max( inSelectedBoxMax.x, localBoxMax.x );
		inSelectedBoxMax.y = std::max( inSelectedBoxMax.y, localBoxMax.y );
	}

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
	}
}

template void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate<Cyclone::UI::EViewportType::TopXZ>( Cyclone::Core::LevelInterface * );
template void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate<Cyclone::UI::EViewportType::FrontXY>( Cyclone::Core::LevelInterface * );
template void Cyclone::UI::Tool::SelectionTransformTool::OnUpdate<Cyclone::UI::EViewportType::SideYZ>( Cyclone::Core::LevelInterface * );

template void Cyclone::UI::Tool::SelectionTransformTool::OnDraw<Cyclone::UI::EViewportType::TopXZ>( Cyclone::Core::LevelInterface * );
template void Cyclone::UI::Tool::SelectionTransformTool::OnDraw<Cyclone::UI::EViewportType::FrontXY>( Cyclone::Core::LevelInterface * );
template void Cyclone::UI::Tool::SelectionTransformTool::OnDraw<Cyclone::UI::EViewportType::SideYZ>( Cyclone::Core::LevelInterface * );