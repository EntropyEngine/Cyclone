#include "pch.h"
#include "Cyclone/UI/Outliner.hpp"

// Cyclone Utils
#include "Cyclone/Util/String.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone Components
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
	if ( ImGui::CollapsingHeader( "Outliner", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 0.0f }, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y } );
		if ( ImGui::BeginChild( "OutlinerChild", { 0.0f, 256.0f }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking ) ) {
			if ( ImGui::BeginTable( "OutlinerTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, { 0.0f, -0.0f } ) )
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
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				for ( const auto &[entityCategory, typeMap] : outlinerTree ) {
					ImGui::TableNextRow();

					bool *categoryVisible = inLevelInterface->GetEntityCtx().GetEntityCategoryIsVisible( entityCategory );
					bool *categorySelectable = inLevelInterface->GetEntityCtx().GetEntityCategoryIsSelectable( entityCategory );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
					if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##cV", entityCategory ), reinterpret_cast<bool *>( categoryVisible ) ) );

					ImGui::TableSetColumnIndex( 2 );
					if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##cS", entityCategory ), reinterpret_cast<bool *>( categorySelectable ) ) );
					ImGui::PopStyleVar( 1 );

					ImGui::TableSetColumnIndex( 0 );
					if ( ImGui::TreeNodeEx( inLevelInterface->GetEntityCtx().GetEntityCategoryName( entityCategory ), treeNodeFlags ) ) {
						for ( const auto &[entityType, entityList] : typeMap ) {
							ImGui::TableNextRow();

							bool *entityTypeVisible = inLevelInterface->GetEntityCtx().GetEntityTypeIsVisible( entityType );
							bool *entityTypeSelectable = inLevelInterface->GetEntityCtx().GetEntityTypeIsSelectable( entityType );

							ImGui::TableSetColumnIndex( 1 );
							ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
							if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##tV", entityType ), reinterpret_cast<bool *>( entityTypeVisible ) ) );

							ImGui::TableSetColumnIndex( 2 );
							if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##tS", entityType ), reinterpret_cast<bool *>( entityTypeSelectable ) ) );
							ImGui::PopStyleVar( 1 );

							ImGui::TableSetColumnIndex( 0 );
							if ( ImGui::TreeNodeEx( inLevelInterface->GetEntityCtx().GetEntityTypeName( entityType ), treeNodeFlags ) ) {
								ImGuiListClipper clipper;
								clipper.Begin( entityList.size() );

								while ( clipper.Step() ) {

									for ( int rowN = clipper.DisplayStart; rowN < clipper.DisplayEnd; ++rowN ) {
										ImGui::PushID( rowN );

										const auto entity = entityList[rowN];

										ImGui::TableNextRow();
										ImGui::TableSetColumnIndex( 0 );

										ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
										ImGuiTreeNodeFlags treeLeafFlags = ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_AllowOverlap;

										bool entityInSelection = selectionContext.GetSelectedEntities().contains( entity );
										bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

										auto &entityVisible = registry.get<Cyclone::Core::Component::Visible>( entity );
										auto &entitySelectable = registry.get<Cyclone::Core::Component::Selectable>( entity );

										if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
										if ( !static_cast<bool>( entityVisible ) || !*categoryVisible || !*entityTypeVisible ) selectionFlags |= ImGuiSelectableFlags_Disabled;
										if ( !static_cast<bool>( entitySelectable ) || !*categorySelectable || !*entityTypeSelectable ) selectionFlags |= ImGuiSelectableFlags_Disabled;

										if ( !( selectionFlags & ImGuiSelectableFlags_Disabled ) ) treeLeafFlags |= ImGuiTreeNodeFlags_Bullet;

										const auto entityIdString = Cyclone::Util::PrefixString( "##b", entity );
										ImGui::TreeNodeEx( entityIdString, treeLeafFlags );

										//ImGui::Bullet();
										ImGui::SameLine( 0, 0 );
										ImGui::SetNextItemAllowOverlap();
										if ( ImGui::Selectable( entityIdString.Value(), entityInSelection, selectionFlags ) ) {
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
										if ( ImGui::Checkbox( "##V", reinterpret_cast<bool *>( &entityVisible ) ) ) selectionContext.DeselectEntity( entity );

										ImGui::TableSetColumnIndex( 2 );
										if ( ImGui::Checkbox( "##S", reinterpret_cast<bool *>( &entitySelectable ) ) ) selectionContext.DeselectEntity( entity );
										ImGui::PopStyleVar( 1 );

										ImGui::TreePop();
										ImGui::PopID();
									}
								}
								ImGui::TreePop();
							}
						}
						ImGui::TreePop();
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	/*
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
	*/
}
