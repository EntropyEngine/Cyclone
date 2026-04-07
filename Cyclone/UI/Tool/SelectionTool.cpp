#include "pch.h"
#include "Cyclone/UI/Tool/SelectionTool.hpp"

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

void Cyclone::UI::Tool::SelectionTool::OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	ImGuiIO &io = ImGui::GetIO();

	const bool isLeftClickShort = ( ImGui::IsMouseReleased( 0, inViewportData.mCanvasID ) || ImGui::IsMouseReleased( 0, ImGuiKeyOwner_NoOwner ) ) && io.MouseDownDurationPrev[0] < io.MouseDoubleClickTime;
	if ( !inViewportData.mIsActive || !isLeftClickShort || ImGuizmo::IsOver() || ImGuizmo::IsUsingAny() ) return;

	auto &entityManager = inLevelInterface->GetEntityManager();
	if ( !entityManager.CanAquireActionLock() ) return;

	switch ( inType ) {
		case EViewportType::TopXZ: OnUpdate<EViewportType::TopXZ>( inLevelInterface, inViewportData ); break;
		case EViewportType::FrontXY: OnUpdate<EViewportType::FrontXY>( inLevelInterface, inViewportData ); break;
		case EViewportType::SideYZ: OnUpdate<EViewportType::SideYZ>( inLevelInterface, inViewportData ); break;
		case EViewportType::Perspective: OnUpdatePerspective( inLevelInterface, inViewportData ); break;
	}
}

void Cyclone::UI::Tool::SelectionTool::OnUpdatePerspective( Cyclone::Core::LevelInterface * inLevelInterface, const ViewportData & inViewportData )
{
	auto &entityManager = inLevelInterface->GetEntityManager();

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

	entt::entity hovered = inViewportData.mEntityIndexShader->ReadViewport( inViewportData.mDeviceContext, inViewportData.mEntitySRV, static_cast<size_t>( inViewportData.mAbsoluteMouse.x ), static_cast<size_t>( inViewportData.mAbsoluteMouse.y ) );
	if ( hovered != entt::null ) {
		selectionCandidates.insert( hovered );
	}

	std::erase_if( selectionCandidates, [&cregistry, &entityManager]( entt::entity inEntity ) {
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Selectable>( inEntity ) ) ) return true;
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Visible>( inEntity ) ) ) return true;

		const auto entityType = cregistry.get<Cyclone::Core::Component::EntityType>( inEntity );
		const auto entityCategory = cregistry.get<Cyclone::Core::Component::EntityCategory>( inEntity );

		if ( !entityManager.GetEntityTypeIsVisible( entityType ) ) return true;
		if ( !entityManager.GetEntityTypeIsSelectable( entityType ) ) return true;

		if ( !entityManager.GetEntityCategoryIsVisible( entityCategory ) ) return true;
		if ( !entityManager.GetEntityCategoryIsSelectable( entityCategory ) ) return true;

		return false;
	} );

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

			if ( ctrlHeld && previousSelectedEntities.contains( *it ) ) selectionContext.DeselectEntity( *it );
			else selectionContext.AddSelectedEntity( *it );
		}
		else {
			if ( ctrlHeld && previousSelectedEntities.contains( *selectionCandidates.begin() ) ) selectionContext.DeselectEntity( *selectionCandidates.begin() );
			else selectionContext.AddSelectedEntity( *selectionCandidates.begin() );
		}
	}

	selectionContext.mPreviousCandidates = selectionCandidates;

	if ( selectionContext.mDirty && entityManager.IsSelectionModified() ) {
		entityManager.BeginAction();
		entityManager.EndAction( registry );

		auto &transfromContext = inLevelInterface->GetSelectionTransformCtx();
		transfromContext.OnPreUpdate();
		for ( const entt::entity entity : selectionContext.GetSelectedEntities() ) {
			const auto &[position, box] = registry.get<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>( entity );

			transfromContext.IncludeSelectedEntity( position, box );
		}
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::SelectionTool::OnUpdate( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	auto &entityManager = inLevelInterface->GetEntityManager();

	bool ctrlHeld = ImGui::IsKeyDown( ImGuiMod_Ctrl );
	bool shiftHeld = ImGui::IsKeyDown( ImGuiMod_Shift );

	if ( !( ctrlHeld || shiftHeld ) ) {}

	auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();
	const auto &gridContext = inLevelInterface->GetGridCtx();

	// Not a reference; need original
	const std::set<entt::entity> previousSelectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity previousSelectedEntity = selectionContext.GetSelectedEntity();
	std::set<entt::entity> selectionCandidates;

	double clickPositionD[4] = { 0, 0, 0, 0 };
	clickPositionD[ViewportTypeTraits<T>::AxisU] = inViewportData.mWorldMouseU;
	clickPositionD[ViewportTypeTraits<T>::AxisV] = inViewportData.mWorldMouseV;
	Cyclone::Math::Vector4D clickPosition = Cyclone::Math::Vector4D::sLoad( clickPositionD );
	Cyclone::Math::Vector4D clickExtent = Cyclone::Math::Vector4D::sZeroSetValueByIndex<ViewportTypeTraits<T>::AxisW>( gridContext.mWorldLimit );
	Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> clickBox{ .mCenter = clickPosition, .mExtent = clickExtent };

	double handleRadius = 2.0f * Cyclone::Core::Editor::GridContext::kPositionHandleSize * orthographicContext.mZoomScale2D;
	Cyclone::Math::Vector4D entityExtent = Cyclone::Math::Vector4D::sReplicate( handleRadius );

	entt::registry &registry = inLevelInterface->GetRegistry();
	const entt::registry &cregistry = registry;
	auto view = cregistry.view<Cyclone::Core::Component::Position>();
	for ( const entt::entity entity : view ) {
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity ).mValue;
		Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> enitityHandle{ .mCenter = position, .mExtent = entityExtent };

		if ( enitityHandle.Intersects( clickBox ) ) {
			selectionCandidates.insert( entity );
		}
	}

	entt::entity hovered = inViewportData.mEntityIndexShader->ReadViewport( inViewportData.mDeviceContext, inViewportData.mEntitySRV, static_cast<size_t>( inViewportData.mAbsoluteMouse.x ), static_cast<size_t>( inViewportData.mAbsoluteMouse.y ) );
	if ( hovered != entt::null ) {
		selectionCandidates.insert( hovered );
	}

	std::erase_if( selectionCandidates, [&cregistry, &entityManager]( entt::entity inEntity ) {
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Selectable>( inEntity ) ) ) return true;
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Visible>( inEntity ) ) ) return true;

		const auto entityType = cregistry.get<Cyclone::Core::Component::EntityType>( inEntity );
		const auto entityCategory = cregistry.get<Cyclone::Core::Component::EntityCategory>( inEntity );

		if ( !entityManager.GetEntityTypeIsVisible( entityType ) ) return true;
		if ( !entityManager.GetEntityTypeIsSelectable( entityType ) ) return true;

		if ( !entityManager.GetEntityCategoryIsVisible( entityCategory ) ) return true;
		if ( !entityManager.GetEntityCategoryIsSelectable( entityCategory ) ) return true;

		return false;
	} );

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

			if ( ctrlHeld && previousSelectedEntities.contains( *it ) ) selectionContext.DeselectEntity( *it );
			else selectionContext.AddSelectedEntity( *it );
		}
		else {
			if ( ctrlHeld && previousSelectedEntities.contains( *selectionCandidates.begin() ) ) selectionContext.DeselectEntity( *selectionCandidates.begin() );
			else selectionContext.AddSelectedEntity( *selectionCandidates.begin() );
		}
	}

	selectionContext.mPreviousCandidates = selectionCandidates;

	if ( selectionContext.mDirty && entityManager.IsSelectionModified() ) {
		entityManager.BeginAction();
		entityManager.EndAction( registry );

		auto &transfromContext = inLevelInterface->GetSelectionTransformCtx();
		transfromContext.OnPreUpdate();
		for ( const entt::entity entity : selectionContext.GetSelectedEntities() ) {
			const auto &[position, box] = registry.get<Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>( entity );

			transfromContext.IncludeSelectedEntity( position, box );
		}
	}
}

//template void Cyclone::UI::Tool::SelectionTool::OnUpdate<Cyclone::UI::EViewportType::TopXZ>( Cyclone::Core::LevelInterface *, const ViewportData & );
//template void Cyclone::UI::Tool::SelectionTool::OnUpdate<Cyclone::UI::EViewportType::FrontXY>( Cyclone::Core::LevelInterface *, const ViewportData & );
//template void Cyclone::UI::Tool::SelectionTool::OnUpdate<Cyclone::UI::EViewportType::SideYZ>( Cyclone::Core::LevelInterface *, const ViewportData & );