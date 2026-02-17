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

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;

		ImGui::TableSetupColumn( "Name" );
		ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
		ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
		ImGui::TableHeadersRow();

		for ( const auto &[entityCategory, typeMap] : outlinerTree ) {
			ImGui::TableNextRow();

			bool v;
			ImGui::TableSetColumnIndex( 1 );
			ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
			if ( ImGui::Checkbox( std::format( "##c{}V", static_cast<size_t>( entityCategory ) ).c_str(), reinterpret_cast<bool *>( &v ) ) );

			ImGui::TableSetColumnIndex( 2 );
			if ( ImGui::Checkbox( std::format( "##c{}S", static_cast<size_t>( entityCategory ) ).c_str(), reinterpret_cast<bool *>( &v ) ) );
			ImGui::PopStyleVar( 1 );

			ImGui::TableSetColumnIndex( 0 );
			if ( ImGui::TreeNodeEx( inLevelInterface->GetEntityCtx().GetEntityCategoryName( entityCategory ), treeNodeFlags ) ) {
				for ( const auto &[entityType, entityList] : typeMap ) {
					ImGui::TableNextRow();

					bool v;
					ImGui::TableSetColumnIndex( 1 );
					ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
					if ( ImGui::Checkbox( std::format( "##t{}V", static_cast<size_t>( entityType ) ).c_str(), reinterpret_cast<bool *>( &v ) ) );

					ImGui::TableSetColumnIndex( 2 );
					if ( ImGui::Checkbox( std::format( "##t{}S", static_cast<size_t>( entityType ) ).c_str(), reinterpret_cast<bool *>( &v ) ) );
					ImGui::PopStyleVar( 1 );

					ImGui::TableSetColumnIndex( 0 );
					if ( ImGui::TreeNodeEx( inLevelInterface->GetEntityCtx().GetEntityTypeName( entityType ), treeNodeFlags ) ) {
						for ( const auto entity : entityList ) {
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex( 0 );

							ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
							ImGuiTreeNodeFlags treeLeafFlags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_AllowOverlap;

							bool entityInSelection = selectionContext.GetSelectedEntities().contains( entity );
							bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

							auto &entityVisible = registry.get<Cyclone::Core::Component::Visible>( entity );
							auto &entitySelectable = registry.get<Cyclone::Core::Component::Selectable>( entity );

							if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
							if ( !static_cast<bool>( entityVisible ) ) selectionFlags |= ImGuiSelectableFlags_Disabled;
							if ( !static_cast<bool>( entitySelectable ) ) selectionFlags |= ImGuiSelectableFlags_Disabled;

							if ( !( selectionFlags & ImGuiSelectableFlags_Disabled ) ) treeLeafFlags |= ImGuiTreeNodeFlags_Bullet;

							ImGui::TreeNodeEx( std::format( "##b{}", static_cast<size_t>( entity ) ).c_str(), treeLeafFlags );

							//ImGui::Bullet();
							ImGui::SameLine( 0, 0 );
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
							if ( ImGui::Checkbox( std::format( "##e{}V", static_cast<size_t>( entity ) ).c_str(), reinterpret_cast<bool *>( &entityVisible ) ) ) selectionContext.DeselectEntity( entity );

							ImGui::TableSetColumnIndex( 2 );
							if ( ImGui::Checkbox( std::format( "##e{}S", static_cast<size_t>( entity ) ).c_str(), reinterpret_cast<bool *>( &entitySelectable ) ) ) selectionContext.DeselectEntity( entity );
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
		auto &&other = cregistry.storage<Cyclone::Core::Component::Position>( "delta"_hs );
		auto view2 = view | entt::basic_view{ *other };

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
