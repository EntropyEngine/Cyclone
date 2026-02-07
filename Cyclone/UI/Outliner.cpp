#include "pch.h"
#include "Cyclone/UI/Outliner.hpp"

// Cyclone Core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_internal.h>

// STL
#include <format>

void Cyclone::UI::Outliner::Update( Cyclone::Core::LevelInterface *inEntityInterface )
{
	if ( ImGui::BeginTable( "Entity List", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) ) {

		ImGui::TableSetupColumn( "ID" );
		ImGui::TableSetupColumn( "Type" );
		ImGui::TableSetupColumn( "Position" );
		ImGui::TableHeadersRow();

		const entt::registry &cregistry = inEntityInterface->GetRegistry();
		cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position>().each( [&inEntityInterface]( const entt::entity inEntity, const Cyclone::Core::Component::EntityType &inEntityType, const Cyclone::Core::Component::Position &inPosition ) {
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex( 0 );
			ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
			if ( inEntityInterface->GetSelectedEntity() == inEntity ) selectionFlags |= ImGuiSelectableFlags_Highlight;

			if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( inEntity ) ).c_str(), inEntityInterface->GetSelectedEntities().contains( inEntity ), selectionFlags ) ) {
				if ( ImGui::GetIO().KeyCtrl ) {
					if ( inEntityInterface->GetSelectedEntity() == inEntity ) {
						inEntityInterface->DeselectEntity( inEntity );
					}
					else {
						inEntityInterface->AddSelectedEntity( inEntity );
					}
				}
				else {
					inEntityInterface->SetSelectedEntity( inEntity );
				}
			};

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( Cyclone::Core::Entity::EntityTypeRegistry::GetEntityTypeName( inEntityType ) );

			ImGui::TableSetColumnIndex( 2 );
			ImGui::Text( "% 7.2f\n% 7.2f\n% 7.2f", inPosition.GetX(), inPosition.GetY(), inPosition.GetZ() );
		} );

		ImGui::EndTable();
	}
}
