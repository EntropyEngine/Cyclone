#include "pch.h"
#include "Cyclone/UI/Tool/PathSelectionTool.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone Rendering
#include "Cyclone/Rendering/Shader/EntityIndexShader.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

// ImGui Includes
#include <imgui_internal.h>

// ImGuizmo
#include <ImGuizmo/ImGuizmo.h>

void Cyclone::UI::Tool::PathSelectionTool::OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	ImGuiIO &io = ImGui::GetIO();

	const bool isLeftClickShort = ( ImGui::IsMouseReleased( 0, inViewportData.mCanvasID ) || ImGui::IsMouseReleased( 0, ImGuiKeyOwner_NoOwner ) ) && io.MouseDownDurationPrev[0] < io.MouseDoubleClickTime;
	if ( !inViewportData.mIsActive || !isLeftClickShort || ImGuizmo::IsOver() || ImGuizmo::IsUsingAny() || !mIsSelected ) return;

	auto &entityManager = inLevelInterface->GetEntityManager();
	if ( !entityManager.CanAquireActionLock() ) return;

	bool ctrlHeld = ImGui::IsKeyDown( ImGuiMod_Ctrl );
	bool shiftHeld = ImGui::IsKeyDown( ImGuiMod_Shift );

	auto &selectionContext = inLevelInterface->GetSelectionCtx();

	// Not a reference; need original
	const std::set<entt::entity> previousSelectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity previousSelectedEntity = selectionContext.GetSelectedEntity();

	std::set<entt::entity> modifiedEntities;

	entt::registry &registry = inLevelInterface->GetRegistry();
	const entt::registry &cregistry = registry;

	std::vector<entt::entity> hovered = inViewportData.mEntityIndexShader->ReadOrderedEntities( inViewportData.mDeviceContext, inViewportData.mEntitySRV, static_cast<size_t>( inViewportData.mAbsoluteMouse.x ), static_cast<size_t>( inViewportData.mAbsoluteMouse.y ) );
	std::erase_if( hovered, [&cregistry, &entityManager]( entt::entity inEntity ) {
		inEntity = static_cast<entt::entity>( entt::to_entity( inEntity ) );

		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Selectable>( inEntity ) ) ) return true;
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Visible>( inEntity ) ) ) return true;

		const auto entityType = cregistry.get<Cyclone::Core::Component::EntityType>( inEntity );
		const auto entityCategory = cregistry.get<Cyclone::Core::Component::EntityCategory>( inEntity );

		if ( static_cast<entt::id_type>( entityCategory ) != "path"_hs.value() ) return true;

		if ( !entityManager.GetEntityTypeIsVisible( entityType ) ) return true;
		if ( !entityManager.GetEntityTypeIsSelectable( entityType ) ) return true;

		if ( !entityManager.GetEntityCategoryIsVisible( entityCategory ) ) return true;
		if ( !entityManager.GetEntityCategoryIsSelectable( entityCategory ) ) return true;

		return false;
	} );

	auto view = registry.view<Cyclone::Core::Component::PathSelection>();

	if ( hovered.empty() ) {
		if ( !shiftHeld && !ctrlHeld ) {
			// Nuke selection
			// Find all paths, nuke their selections

			selectionContext.ClearSelection();

			for ( entt::entity entity : view ) {
				if ( view.get<Cyclone::Core::Component::PathSelection>( entity ).ClearSelection() ) {
					modifiedEntities.insert( entity );
				}
			}
		}
	}
	else {
		hovered.resize( 1 );

		if ( !shiftHeld && !ctrlHeld ) {
			selectionContext.ClearSelection();

			for ( entt::entity entity : view ) {
				entt::entity rootEntity = static_cast<entt::entity>( entt::to_entity( hovered[0] ) );
				if ( entity != rootEntity ) {
					if ( view.get<Cyclone::Core::Component::PathSelection>( entity ).ClearSelection() ) {
						modifiedEntities.insert( entity );
					}
				}
				else {
					if ( view.get<Cyclone::Core::Component::PathSelection>( entity ).SetSelectedKnot( entt::to_version( hovered[0] ) ) ) {
						modifiedEntities.insert( entity );
					}
					selectionContext.SetSelectedEntity( entity );
				}
			}
		}
		else if ( shiftHeld ) {
			entt::entity firstEntity = static_cast<entt::entity>( entt::to_entity( hovered[0] ) );
			uint16_t firstKnot = entt::to_version( hovered[0] );

			if ( view.get<Cyclone::Core::Component::PathSelection>( firstEntity ).AddSelectedKnot( firstKnot ) ) {
				modifiedEntities.insert( firstEntity );
			}
			selectionContext.AddSelectedEntity( firstEntity );
		}
		else if ( ctrlHeld ) {
			entt::entity firstEntity = static_cast<entt::entity>( entt::to_entity( hovered[0] ) );
			uint16_t firstKnot = entt::to_version( hovered[0] );

			auto &pathSelection = view.get<Cyclone::Core::Component::PathSelection>( firstEntity );

			if ( pathSelection.mCurrentKnot == firstKnot && previousSelectedEntity == firstEntity ) {
				pathSelection.DeselectKnot( firstKnot );
				modifiedEntities.insert( firstEntity );
			}
			else {
				pathSelection.AddSelectedKnot( firstKnot );
				modifiedEntities.insert( firstEntity );
			}

			if ( pathSelection.mSelectedKnots.size() ) {
				selectionContext.AddSelectedEntity( firstEntity );
			}
			else {
				selectionContext.DeselectEntity( firstEntity );
			}
		}
	}

	// Check if we have any modified entities OR our selection has changed
	if ( modifiedEntities.size() || entityManager.IsSelectionModified() ) {
		entityManager.BeginAction();

		for ( entt::entity entity : modifiedEntities ) {
			entityManager.UpdateEntity( entity, registry );
		}

		entityManager.EndAction( registry );

		auto &transfromContext = inLevelInterface->GetSelectionTransformCtx();
		transfromContext.OnPreUpdate();
		for ( const entt::entity entity : selectionContext.GetSelectedEntities() ) {
			const auto &[position, box] = registry.get<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>( entity );

			transfromContext.IncludeSelectedEntity( position, box );
		}
	}
}
