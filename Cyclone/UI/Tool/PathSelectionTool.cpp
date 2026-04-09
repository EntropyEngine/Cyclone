#include "pch.h"
#include "Cyclone/UI/Tool/PathSelectionTool.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"
#include "Cyclone/Core/Component/Visible.hpp"

// Cyclone Rendering
#include "Cyclone/Rendering/Shader/EntityIndexShader.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

// ImGui Includes
#include <imgui_internal.h>

// ImGuizmo
#include <ImGuizmo/ImGuizmo.h>

void Cyclone::UI::Tool::PathSelectionTool::OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface * inLevelInterface, const ViewportData & inViewportData )
{
	ImGuiIO &io = ImGui::GetIO();

	const bool isLeftClickShort = ( ImGui::IsMouseReleased( 0, inViewportData.mCanvasID ) || ImGui::IsMouseReleased( 0, ImGuiKeyOwner_NoOwner ) ) && io.MouseDownDurationPrev[0] < io.MouseDoubleClickTime;
	if ( !inViewportData.mIsActive || !isLeftClickShort || ImGuizmo::IsOver() || ImGuizmo::IsUsingAny() || !mIsSelected ) return;

	auto &entityManager = inLevelInterface->GetEntityManager();
	if ( !entityManager.CanAquireActionLock() ) return;

	bool ctrlHeld = ImGui::IsKeyDown( ImGuiMod_Ctrl );
	bool shiftHeld = ImGui::IsKeyDown( ImGuiMod_Shift );

	if ( !( ctrlHeld || shiftHeld ) ) {}

	auto &selectionContext = inLevelInterface->GetSelectionCtx();

	// Not a reference; need original
	const std::set<entt::entity> previousSelectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity previousSelectedEntity = selectionContext.GetSelectedEntity();
	std::set<entt::entity> selectionCandidates;

	entt::registry &registry = inLevelInterface->GetRegistry();
	const entt::registry &cregistry = registry;

	entt::entity hovered = inViewportData.mEntityIndexShader->ReadClosestEntity( inViewportData.mDeviceContext, inViewportData.mEntitySRV, static_cast<size_t>( inViewportData.mAbsoluteMouse.x ), static_cast<size_t>( inViewportData.mAbsoluteMouse.y ) );
	if ( hovered != entt::null ) {
		selectionCandidates.insert( hovered );
	}

	auto nuke = [&cregistry, &entityManager]( entt::entity inEntity ) {
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
	};

	std::erase_if( selectionCandidates, nuke );

	if ( shiftHeld ) {
		for ( auto e : selectionCandidates ) {
			selectionContext.AddSelectedEntity( e );
		}
		if ( previousSelectedEntity != entt::null ) {
			selectionContext.AddSelectedEntity( previousSelectedEntity );
		}
	}
	else if ( ctrlHeld ) {
	}
	else {
		selectionContext.ClearSelection();
	}

	if ( !selectionCandidates.empty() ) {
		
		if ( selectionCandidates == selectionContext.mPreviousCandidates ) {
			auto it = selectionCandidates.upper_bound( previousSelectedEntity );
			if ( it == selectionCandidates.end() ) {
				it = selectionCandidates.begin();
			}

			if ( ctrlHeld && previousSelectedEntities.contains( *it ) ) {
				selectionContext.DeselectEntity( *it );
			}
			else {
				selectionContext.AddSelectedEntity( static_cast<entt::entity>( entt::to_entity( *it ) ) );
				selectionContext.AddSelectedEntity( *it );
			}
		}
		else {
			if ( ctrlHeld && previousSelectedEntities.contains( *selectionCandidates.begin() ) ) {
				selectionContext.DeselectEntity( *selectionCandidates.begin() );
			}
			else {
				selectionContext.AddSelectedEntity( static_cast<entt::entity>( entt::to_entity( *selectionCandidates.begin() ) ) );
				selectionContext.AddSelectedEntity( *selectionCandidates.begin() );
			}
		}
		
	}

	selectionContext.mPreviousCandidates = selectionCandidates;

	std::erase_if( selectionContext.mSelectedEntities, nuke );
	if ( !selectionContext.mSelectedEntities.contains( selectionContext.mSelectedEntity ) ) {
		auto it = selectionContext.mSelectedEntities.begin();

		if ( it != selectionContext.mSelectedEntities.end() ) {
			selectionContext.mSelectedEntity = *it;
		}
		else {
			selectionContext.mSelectedEntity = entt::null;
		}
	}

	if ( selectionContext.mDirty && entityManager.IsSelectionModified() ) {
		entityManager.BeginAction();
		entityManager.EndAction( registry );

		auto &transfromContext = inLevelInterface->GetSelectionTransformCtx();
		transfromContext.OnPreUpdate();
		for ( const entt::entity entity : selectionContext.GetSelectedEntities() ) {
			if ( entt::to_version( entity ) == 0 ) {
				const auto &[position, box] = registry.get<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>( entity );
				transfromContext.IncludeSelectedEntity( position, box );
			}
		}
	}
}
