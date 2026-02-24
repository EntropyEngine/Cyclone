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
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &entityContext = inLevelInterface->GetEntityCtx();
	entt::registry &registry = inLevelInterface->GetRegistry();

	ImGuiChildFlags sectionChildFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY;
	ImGuiWindowFlags sectionWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
	ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_NoBordersInBody;
	ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_FramePadding;

	float origHeight = ImGui::GetContentRegionAvail().y;

	if ( ImGui::CollapsingHeader( "Outliner", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		RebuildTree( inLevelInterface );
		ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 32.0f }, { ImGui::GetContentRegionAvail().x, mOutlinerHeight + mRemainingHeight } );
		if ( ImGui::BeginChild( "OutlinerChild", { 0.0f, 256.0f }, sectionChildFlags, sectionWindowFlags ) ) {
			if ( ImGui::BeginTable( "OutlinerTable", 3, tableFlags, { 0.0f, -1.0f } ) ) {

				ImGui::TableSetupColumn( "Name" );
				ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, { 0.0f, 0.0f } );

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
								clipper.Begin( static_cast<int>( entityList.size() ) );

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
										ImGui::TreeNodeEx( entityIdString, treeLeafFlags | ImGuiTreeNodeFlags_FramePadding );

										//ImGui::Bullet();
										ImGui::SameLine( 0, 0 );
										ImGui::SetCursorPosY( ImGui::GetCursorPosY() - style.FramePadding.y );
										ImGui::SetNextItemAllowOverlap();
										ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.0f, 0.5f } );
										if ( ImGui::Selectable( entityIdString.Value(), entityInSelection, selectionFlags, { 0, style.FramePadding.y * 2 + ImGui::GetTextLineHeight() } ) ) {
											if ( io.KeyCtrl ) {
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
										ImGui::PopStyleVar( 1 );

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

				ImGui::PopStyleVar( 1 );

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		mOutlinerHeight = ImGui::GetItemRectSize().y;
	}


	if ( ImGui::CollapsingHeader( "Selection", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		auto view = registry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Visible, Cyclone::Core::Component::Selectable>();
		ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 32.0f }, { ImGui::GetContentRegionAvail().x, mSelectionHeight + mRemainingHeight } );
		if ( ImGui::BeginChild( "SelectionChild", { 0.0f, 256.0f }, sectionChildFlags, sectionWindowFlags ) ) {
			if ( ImGui::BeginTable( "SelectionTable", 4, tableFlags, { 0.0f, -1.0f } ) ) {
				ImGui::TableSetupColumn( "Type" );
				ImGui::TableSetupColumn( "Name" );
				ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, { 0.0f, 0.0f } );

				// Explicitly create copy rather than ref
				std::vector<entt::entity> previousSelection( selectionContext.GetSelectedEntities().size() );
				std::copy( selectionContext.GetSelectedEntities().begin(), selectionContext.GetSelectedEntities().end(), previousSelection.begin() );
				//const auto previousSelection = selectionContext.GetSelectedEntities();

				for ( entt::entity entity : previousSelection ) {
					ImGui::PushID( static_cast<int>( entity ) );

					bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

					ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
					if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex( 0 );

					const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
					ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.0f, 0.5f } );
					ImGui::SetNextItemAllowOverlap();
					if ( ImGui::Selectable( entityContext.GetEntityTypeName( entityType ), true, selectionFlags, { 0, style.FramePadding.y * 2 + ImGui::GetTextLineHeight() } ) ) {
						if ( io.KeyCtrl ) {
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
					ImGui::PopStyleVar( 1 );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::AlignTextToFramePadding();
					ImGui::Text( Cyclone::Util::PrefixString( "", entity ) );

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

				ImGui::PopStyleVar( 1 );

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		mSelectionHeight = ImGui::GetItemRectSize().y;
	}

	if ( ImGui::CollapsingHeader( "Undo History", ImGuiTreeNodeFlags_DefaultOpen ) ) {
		auto view = registry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Visible, Cyclone::Core::Component::Selectable>();
		ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 32.0f }, { ImGui::GetContentRegionAvail().x, mUndoHistoryHeight + mRemainingHeight } );
		if ( ImGui::BeginChild( "UndoHistoryChild", { 0.0f, 256.0f }, sectionChildFlags, sectionWindowFlags ) ) {
			if ( ImGui::BeginTable( "UndoHistoryTable", 4, tableFlags, { 0.0f, -1.0f } ) ) {

				ImGui::TableSetupColumn( "Epoch" );
				ImGui::TableSetupColumn( "Total" );
				ImGui::TableSetupColumn( "Created" );
				ImGui::TableSetupColumn( "Updated" );
				ImGui::TableSetupScrollFreeze( 0, 1 );
				ImGui::TableHeadersRow();

				const auto &undoStack = inLevelInterface->GetEntityCtx().GetUndoStack();
				const size_t currentEpoch = inLevelInterface->GetEntityCtx().GetUndoEpoch();
				size_t chosenEpoch = currentEpoch;

				for ( size_t epoch = 0; epoch < undoStack.size(); ++epoch ) {
					ImGui::PushID( static_cast<int>( epoch ) );

					const entt::registry &epochRegistry = undoStack[epoch];

					size_t nChanges = epochRegistry.view<entt::entity>().size();
					size_t nUpdates = epochRegistry.view<Cyclone::Core::Component::EpochNumber>().size();

					bool isCurrent = epoch == currentEpoch;
					bool disabled = epoch > currentEpoch;

					if ( disabled ) ImGui::PushStyleColor( ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex( 0 );
					if ( ImGui::Selectable( Cyclone::Util::PrefixString( "", epoch ), isCurrent, ImGuiSelectableFlags_SpanAllColumns ) ) {
						chosenEpoch = epoch;
					};

					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( Cyclone::Util::PrefixString( "", nChanges ) );

					ImGui::TableSetColumnIndex( 2 );
					ImGui::Text( Cyclone::Util::PrefixString( "", nChanges - nUpdates ) );

					ImGui::TableSetColumnIndex( 3 );
					ImGui::Text( Cyclone::Util::PrefixString( "", nUpdates ) );

					if ( disabled ) ImGui::PopStyleColor( 1 );

					ImGui::PopID();
				}

				if ( chosenEpoch != currentEpoch ) {
					while ( inLevelInterface->GetEntityCtx().GetUndoEpoch() > chosenEpoch ) {
						inLevelInterface->GetEntityCtx().UndoAction( inLevelInterface->GetRegistry() );
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		mUndoHistoryHeight = ImGui::GetItemRectSize().y;
	}

	mRemainingHeight = std::max( -std::sqrt( std::abs( ImGui::GetContentRegionAvail().y ) ), ImGui::GetContentRegionAvail().y);
}

void Cyclone::UI::Outliner::RebuildTree( const Cyclone::Core::LevelInterface *inLevelInterface )
{
	// Clear all entity lists
	for ( auto &[entityCategory, typeMap] : mOutlinerTree ) {
		for ( auto &[entityType, entityList] : typeMap ) {
			entityList.clear();
		}
	}

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
