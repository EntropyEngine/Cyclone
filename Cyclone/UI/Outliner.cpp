#include "pch.h"
#include "Cyclone/UI/Outliner.hpp"

// Cyclone Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_internal.h>

// STL
#include <format>

void Cyclone::UI::Outliner::Update( Cyclone::Core::LevelInterface *inLevelInterface )
{
	if ( ImGui::BeginTable( "Outliner", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) )
	{
		std::map<Cyclone::Core::Component::EntityCategory, std::map<Cyclone::Core::Component::EntityType, std::vector<entt::entity>>> outlinerTree;

		auto &selectionContext = inLevelInterface->GetSelectionCtx();

		entt::registry &registry = inLevelInterface->GetRegistry();
		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::EntityCategory>();
		for ( const entt::entity entity : view ) {
			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &entityCategory = view.get<Cyclone::Core::Component::EntityCategory>( entity );

			auto itCategory = outlinerTree.find( entityCategory );
			if ( itCategory == outlinerTree.end() ) {
				itCategory = outlinerTree.emplace( entityCategory, std::map<Cyclone::Core::Component::EntityType, std::vector<entt::entity>>{} ).first;
			}

			auto itType = itCategory->second.find( entityType );
			if ( itType == itCategory->second.end() ) {
				itType = itCategory->second.emplace( entityType, std::vector<entt::entity>{} ).first;
			}

			itType->second.push_back( entity );
		}

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_LabelSpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;

		ImGui::TableSetupColumn( "Name" );
		ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
		ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
		ImGui::TableHeadersRow();

		for ( const auto &[entityCategory, typeMap] : outlinerTree ) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex( 0 );
			if ( ImGui::TreeNodeEx( inLevelInterface->GetEntityCtx().GetEntityCategoryName( entityCategory ), treeNodeFlags ) ) {
				for ( const auto &[entityType, entityList] : typeMap ) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex( 0 );
					if ( ImGui::TreeNodeEx( inLevelInterface->GetEntityCtx().GetEntityTypeName( entityType ), treeNodeFlags ) ) {
						for ( const auto entity : entityList ) {
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex( 0 );
							ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;

							bool entityInSelection = selectionContext.GetSelectedEntities().contains( entity );
							bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

							if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
							ImGui::Bullet();
							ImGui::SetNextItemAllowOverlap();
							if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( entity ) ).c_str(), entityInSelection, selectionFlags ) ) {
								if ( ImGui::GetIO().KeyCtrl ) {
									if ( entityIsSelected ) {
										selectionContext.DeselectEntity( entity );
									}
									else {
										selectionContext.AddSelectedEntity( entity );
									}
								}
								else {
									selectionContext.SetSelectedEntity( entity );
								}
							};

							ImGui::TableSetColumnIndex( 1 );
							ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
							ImGui::Checkbox( std::format( "##{}H", static_cast<size_t>( entity ) ).c_str(), reinterpret_cast<bool *>( &registry.get<Cyclone::Core::Component::Visible>( entity ) ) );

							ImGui::TableSetColumnIndex( 2 );
							ImGui::Checkbox( std::format( "##{}S", static_cast<size_t>( entity ) ).c_str(), reinterpret_cast<bool *>( &registry.get<Cyclone::Core::Component::Selectable>( entity ) ) );
							ImGui::PopStyleVar( 1 );
						}
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}

		ImGui::EndTable();
	}

	if ( ImGui::BeginTable( "Entity List", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) ) {

		ImGui::TableSetupColumn( "ID" );
		ImGui::TableSetupColumn( "Category" );
		ImGui::TableSetupColumn( "Type" );
		ImGui::TableHeadersRow();

		auto &selectionContext = inLevelInterface->GetSelectionCtx();

		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::EntityCategory>();
		for ( const entt::entity entity : view ) {
			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &entityCategory = view.get<Cyclone::Core::Component::EntityCategory>( entity );

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex( 0 );
			ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;

			bool entityInSelection = selectionContext.GetSelectedEntities().contains( entity );
			bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

			if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
			if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( entity ) ).c_str(), entityInSelection, selectionFlags ) ) {
				if ( ImGui::GetIO().KeyCtrl ) {
					if ( entityIsSelected ) {
						selectionContext.DeselectEntity( entity );
					}
					else {
						selectionContext.AddSelectedEntity( entity );
					}
				}
				else {
					selectionContext.SetSelectedEntity( entity );
				}
			};

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( inLevelInterface->GetEntityCtx().GetEntityCategoryName( entityCategory ) );

			ImGui::TableSetColumnIndex( 2 );
			ImGui::Text( inLevelInterface->GetEntityCtx().GetEntityTypeName( entityType ) );
		}

		ImGui::EndTable();
	}

	if ( ImGui::BeginTable( "Entity List 2", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) ) {

		ImGui::TableSetupColumn( "ID" );
		ImGui::TableSetupColumn( "Type" );
		ImGui::TableSetupColumn( "Position" );
		ImGui::TableHeadersRow();

		auto &selectionContext = inLevelInterface->GetSelectionCtx();

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

			bool entityInSelection = selectionContext.GetSelectedEntities().contains( entity );
			bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

			if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
			if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( entity ) ).c_str(), entityInSelection, selectionFlags ) ) {
				if ( ImGui::GetIO().KeyCtrl ) {
					if ( entityIsSelected ) {
						selectionContext.DeselectEntity( entity );
					}
					else {
						selectionContext.AddSelectedEntity( entity );
					}
				}
				else {
					selectionContext.SetSelectedEntity( entity );
				}
			};

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( inLevelInterface->GetEntityCtx().GetEntityTypeName( entityType ) );

			ImGui::TableSetColumnIndex( 2 );
			ImGui::Text( "% 7.2f\n% 7.2f\n% 7.2f", position.GetX(), position.GetY(), position.GetZ() );
		}

		ImGui::EndTable();
	}
}
