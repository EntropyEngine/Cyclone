#include "pch.h"
#include "Cyclone/UI/Tool/SelectionTool.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"
#include "Cyclone/Core/Component/Visible.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::SelectionTool::OnClick( Cyclone::Core::LevelInterface *inLevelInterface, double inWorldSpaceU, double inWorldSpaceV, double inHandleRadius, double inWorldLimit )
{
	bool ctrlHeld = ImGui::IsKeyDown( ImGuiMod_Ctrl );
	bool shiftHeld = ImGui::IsKeyDown( ImGuiMod_Shift );

	if ( !( ctrlHeld || shiftHeld ) ) {}

	auto &selectionContext = inLevelInterface->GetSelectionCtx();

	// Not a reference; need original
	const std::set<entt::entity> previousSelectedEntities = selectionContext.GetSelectedEntities();
	const entt::entity previousSelectedEntity = selectionContext.GetSelectedEntity();
	std::set<entt::entity> selectionCandidates;

	double clickPositionD[4] = { 0, 0, 0, 0 };
	clickPositionD[ViewportTypeTraits<T>::AxisU] = inWorldSpaceU;
	clickPositionD[ViewportTypeTraits<T>::AxisV] = inWorldSpaceV;
	Cyclone::Math::Vector4D clickPosition = Cyclone::Math::Vector4D::sLoad( clickPositionD );
	Cyclone::Math::Vector4D clickExtent = Cyclone::Math::Vector4D::sZeroSetValueByIndex<ViewportTypeTraits<T>::AxisW>( inWorldLimit );
	Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> clickBox{ .mCenter = clickPosition, .mExtent = clickExtent };

	Cyclone::Math::Vector4D entityExtent = Cyclone::Math::Vector4D::sReplicate( inHandleRadius );

	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	auto view = cregistry.view<Cyclone::Core::Component::Position>();
	for ( const entt::entity entity : view ) {
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity ).mValue;
		Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> enitityHandle{ .mCenter = position, .mExtent = entityExtent };

		if ( enitityHandle.Intersects( clickBox ) ) {
			selectionCandidates.insert( entity );
		}
	}

	const auto &entityContext = inLevelInterface->GetEntityCtx();
	std::erase_if( selectionCandidates, [&cregistry, &entityContext]( entt::entity inEntity ) {
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Selectable>( inEntity ) ) ) return true;
		if ( !static_cast<bool>( cregistry.get<Cyclone::Core::Component::Visible>( inEntity ) ) ) return true;

		const auto entityType = cregistry.get<Cyclone::Core::Component::EntityType>( inEntity );
		const auto entityCategory = cregistry.get<Cyclone::Core::Component::EntityCategory>( inEntity );

		if ( !*entityContext.GetEntityTypeIsVisible( entityType ) ) return true;
		if ( !*entityContext.GetEntityTypeIsSelectable( entityType ) ) return true;

		if ( !*entityContext.GetEntityCategoryIsVisible( entityCategory ) ) return true;
		if ( !*entityContext.GetEntityCategoryIsSelectable( entityCategory ) ) return true;

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
}


template void Cyclone::UI::Tool::SelectionTool::OnClick<Cyclone::UI::EViewportType::TopXZ>( Cyclone::Core::LevelInterface *, double, double, double, double );
template void Cyclone::UI::Tool::SelectionTool::OnClick<Cyclone::UI::EViewportType::FrontXY>( Cyclone::Core::LevelInterface *, double, double, double, double );
template void Cyclone::UI::Tool::SelectionTool::OnClick<Cyclone::UI::EViewportType::SideYZ>( Cyclone::Core::LevelInterface *, double, double, double, double );
