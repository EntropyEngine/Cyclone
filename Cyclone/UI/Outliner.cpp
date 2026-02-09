#include "pch.h"
#include "Cyclone/UI/Outliner.hpp"

// Cyclone Core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_internal.h>

// STL
#include <format>

void Cyclone::UI::Outliner::Update( Cyclone::Core::LevelInterface *inLevelInterface )
{
	if ( ImGui::BeginTable( "Entity List", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) ) {

		ImGui::TableSetupColumn( "ID" );
		ImGui::TableSetupColumn( "Type" );
		ImGui::TableSetupColumn( "Position" );
		ImGui::TableHeadersRow();

		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position>();
		for ( const entt::entity entity : view ) {
			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &position = view.get<Cyclone::Core::Component::Position>( entity );

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex( 0 );
			ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;

			bool entityInSelection = inLevelInterface->GetSelectedEntities().contains( entity );
			bool entityIsSelected = inLevelInterface->GetSelectedEntity() == entity;

			if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
			if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( entity ) ).c_str(), entityInSelection, selectionFlags ) ) {
				if ( ImGui::GetIO().KeyCtrl ) {
					if ( entityIsSelected ) {
						inLevelInterface->DeselectEntity( entity );
					}
					else {
						inLevelInterface->AddSelectedEntity( entity );
					}
				}
				else {
					inLevelInterface->SetSelectedEntity( entity );
				}
			};

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( Cyclone::Core::Entity::EntityTypeRegistry::GetEntityTypeName( entityType ) );

			ImGui::TableSetColumnIndex( 2 );
			ImGui::Text( "% 7.2f\n% 7.2f\n% 7.2f", position.GetX(), position.GetY(), position.GetZ() );
		}

		ImGui::EndTable();
	}

	if ( ImGui::BeginTable( "Entity List 2", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) ) {

		ImGui::TableSetupColumn( "ID" );
		ImGui::TableSetupColumn( "Type" );
		ImGui::TableSetupColumn( "Position" );
		ImGui::TableHeadersRow();

		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType>();
		/*entt::view<entt::get_t<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position>> view{};
		const auto &&entityStorage = cregistry.storage<Cyclone::Core::Component::EntityType>();
		const auto &&positionDeltaStorage = cregistry.storage<Cyclone::Core::Component::Position>( "delta"_hs );
		view.storage<0>( entityStorage );
		view.storage<1>( positionDeltaStorage );*/

		auto &&other = cregistry.storage<Cyclone::Core::Component::Position>( "delta"_hs );
		auto view2 = view | entt::basic_view{ *other };

		//auto view2 = view | reg;
		for ( const entt::entity entity : view2 ) {
			const auto &entityType = view2.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &position = view2.get<Cyclone::Core::Component::Position>( entity );

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex( 0 );
			ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;

			bool entityInSelection = inLevelInterface->GetSelectedEntities().contains( entity );
			bool entityIsSelected = inLevelInterface->GetSelectedEntity() == entity;

			if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
			if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( entity ) ).c_str(), entityInSelection, selectionFlags ) ) {
				if ( ImGui::GetIO().KeyCtrl ) {
					if ( entityIsSelected ) {
						inLevelInterface->DeselectEntity( entity );
					}
					else {
						inLevelInterface->AddSelectedEntity( entity );
					}
				}
				else {
					inLevelInterface->SetSelectedEntity( entity );
				}
			};

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( Cyclone::Core::Entity::EntityTypeRegistry::GetEntityTypeName( entityType ) );

			ImGui::TableSetColumnIndex( 2 );
			ImGui::Text( "% 7.2f\n% 7.2f\n% 7.2f", position.GetX(), position.GetY(), position.GetZ() );
		}

		ImGui::EndTable();
	}
}
