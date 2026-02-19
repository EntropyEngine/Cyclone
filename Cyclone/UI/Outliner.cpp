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
	if ( mOutlinerTreeOpen || mCurrentSelectionOpen ) {
		RebuildTree( inLevelInterface );
	}

	auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &entityContext = inLevelInterface->GetEntityCtx();
	entt::registry &registry = inLevelInterface->GetRegistry();

	ImGuiChildFlags sectionChildFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY;
	ImGuiWindowFlags sectionWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
	ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
	ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull;

	if ( ImGui::CollapsingHeader( "Outliner", &mOutlinerTreeOpen, ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 0.0f }, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y } );
		if ( ImGui::BeginChild( "OutlinerChild", { 0.0f, 256.0f }, sectionChildFlags, sectionWindowFlags ) ) {
			if ( ImGui::BeginTable( "OutlinerTable", 3, tableFlags, { 0.0f, -0.0f } ) ) {

				ImGui::TableSetupColumn( "Name" );
				ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				for ( const auto &[entityCategory, typeMap] : mOutlinerTree ) {
					ImGui::TableNextRow();

					bool *categoryVisible = entityContext.GetEntityCategoryIsVisible( entityCategory );
					bool *categorySelectable = entityContext.GetEntityCategoryIsSelectable( entityCategory );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
					if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##cV", entityCategory ), categoryVisible ) );

					ImGui::TableSetColumnIndex( 2 );
					if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##cS", entityCategory ), categorySelectable ) );
					ImGui::PopStyleVar( 1 );

					ImGui::TableSetColumnIndex( 0 );
					if ( ImGui::TreeNodeEx( entityContext.GetEntityCategoryName( entityCategory ), treeNodeFlags ) ) {
						for ( const auto &[entityType, entityList] : typeMap ) {
							if ( entityList.empty() ) continue;

							ImGui::TableNextRow();

							bool *entityTypeVisible = entityContext.GetEntityTypeIsVisible( entityType );
							bool *entityTypeSelectable = entityContext.GetEntityTypeIsSelectable( entityType );

							ImGui::TableSetColumnIndex( 1 );
							ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
							if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##tV", entityType ), entityTypeVisible ) );

							ImGui::TableSetColumnIndex( 2 );
							if ( ImGui::Checkbox( Cyclone::Util::PrefixString( "##tS", entityType ), entityTypeSelectable ) );
							ImGui::PopStyleVar( 1 );

							ImGui::TableSetColumnIndex( 0 );
							if ( ImGui::TreeNodeEx( entityContext.GetEntityTypeName( entityType ), treeNodeFlags ) ) {
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
										}

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

	auto view = registry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Visible, Cyclone::Core::Component::Selectable>();

	if ( ImGui::CollapsingHeader( "Selection", &mCurrentSelectionOpen, ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 0.0f }, { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y } );
		if ( ImGui::BeginChild( "SelectionChild", { 0.0f, 256.0f }, sectionChildFlags, sectionWindowFlags ) ) {
			if ( ImGui::BeginTable( "SelectionTable", 4, tableFlags, { 0.0f, -0.0f } ) ) {
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Name" );
				ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				// Explicitly create copy rather than ref
				const auto previousSelection = selectionContext.GetSelectedEntities();

				for ( entt::entity entity : previousSelection ) {
					ImGui::PushID( static_cast<int>( entity ) );

					ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
					bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

					if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
					const auto entityIdString = Cyclone::Util::PrefixString( "", entity );

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex( 0 );

					const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
					ImGui::SetNextItemAllowOverlap();
					if ( ImGui::Selectable( entityContext.GetEntityTypeName( entityType ), true, selectionFlags ) ) {
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
					}

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( entityIdString );

					auto &entityVisible = view.get<Cyclone::Core::Component::Visible>( entity );
					auto &entitySelectable = view.get<Cyclone::Core::Component::Selectable>( entity );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::PushStyleVarY( ImGuiStyleVar_FramePadding, 0.0f );
					if ( ImGui::Checkbox( "##V", reinterpret_cast<bool *>( &entityVisible ) ) ) selectionContext.DeselectEntity( entity );

					ImGui::TableSetColumnIndex( 3 );
					if ( ImGui::Checkbox( "##S", reinterpret_cast<bool *>( &entitySelectable ) ) ) selectionContext.DeselectEntity( entity );
					ImGui::PopStyleVar( 1 );

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}
}

void Cyclone::UI::Outliner::RebuildTree( const Cyclone::Core::LevelInterface *inLevelInterface )
{
	// Clear all entity lists
	for ( auto &[entityCategory, typeMap] : mOutlinerTree ) {
		for ( auto &[entityType, entityList] : typeMap ) {
			entityList.clear();
		}
	}

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::EntityCategory>();
	for ( const entt::entity entity : view ) {
		const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
		const auto &entityCategory = view.get<Cyclone::Core::Component::EntityCategory>( entity );

		auto itCategory = mOutlinerTree.find( entityCategory );
		if ( itCategory == mOutlinerTree.end() ) {
			itCategory = mOutlinerTree.emplace( entityCategory, EntityTypeTree{} ).first;
		}

		auto itType = itCategory->second.find( entityType );
		if ( itType == itCategory->second.end() ) {
			itType = itCategory->second.emplace( entityType, EntityList{} ).first;
		}

		itType->second.push_back( entity );
	}
}
