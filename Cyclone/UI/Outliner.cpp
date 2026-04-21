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
#include <imgui_internal.h>

// STL
#include <format>
#include <execution>

using Cyclone::Core::Component::EntityType;
using Cyclone::Core::Component::EntityCategory;
using Cyclone::Core::Component::Visible;
using Cyclone::Core::Component::Selectable;
using Cyclone::Core::Component::EpochNumber;

namespace
{
	bool DrawTreeNodeCheckbox( ImGuiStyle &inStyle, const char *inLabel, bool inActive )
	{
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		ImGui::SetCursorPosX( ImGui::GetCursorPosX() - inStyle.FramePadding.x );
		if ( inActive ) {
			ImGui::RenderCheckMark( drawList, { ImGui::GetCursorScreenPos().x + inStyle.FramePadding.x, ImGui::GetCursorScreenPos().y + inStyle.FramePadding.y }, ImGui::GetColorU32( ImGuiCol_CheckMark ), ImGui::GetTextLineHeight() );
		}
		if ( ImGui::InvisibleButton( inLabel, { inStyle.FramePadding.x * 2 + ImGui::GetTextLineHeight(), inStyle.FramePadding.y * 2 + ImGui::GetTextLineHeight() } ) ) return true;
		return false;
	}

	template<typename T, typename P>
	void UpdateBoolPerPredicate( entt::registry &inRegistry, Cyclone::Core::EntityManager &inEntityManager, P inPredicate, bool inSet )
	{
		inEntityManager.BeginAction();
		for ( auto [entity, type, tag] : inRegistry.view<const P, T>().each() ) {
			if ( inPredicate == type ) {
				tag = static_cast<T>( inSet );
				inEntityManager.UpdateEntity( entity, inRegistry );
			}
		}
		inEntityManager.EndAction( inRegistry );
	}

	void HandleEntityClick( entt::registry &inRegistry, Cyclone::Core::EntityManager &inEntityManager, Cyclone::Core::Tool::SelectionToolContext &inSelectionContext, ImGuiIO &inIo, entt::entity inEntity, bool inEntityIsSelected )
	{
		inEntityManager.BeginAction();
		if ( inIo.KeyCtrl ) {
			if ( inEntityIsSelected ) {
				inSelectionContext.DeselectEntity( inEntity );
			}
			else {
				inSelectionContext.AddSelectedEntity( inEntity );
			}
		}
		else {
			inSelectionContext.SetSelectedEntity( inEntity );
		}
		inEntityManager.EndAction( inRegistry );
	}

	void UpdateBoolPerEntity( entt::registry &inRegistry, Cyclone::Core::EntityManager &inEntityManager, entt::entity inEntity, auto &ioTag )
	{
		inEntityManager.BeginAction();
		*reinterpret_cast<bool *>( &ioTag ) ^= true;
		inEntityManager.UpdateEntity( inEntity, inRegistry );
		inEntityManager.EndAction( inRegistry );
	}

	template<typename T>
	void TreePopup( entt::registry &inRegistry, Cyclone::Core::EntityManager &inEntityManager, Cyclone::Core::Tool::SelectionToolContext &inSelectionContext, T inType )
	{
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
		if ( ImGui::BeginPopupContextItem( "Tree Popup" ) ) {
			if ( ImGui::Selectable( "Add children to selection" ) ) {
				inEntityManager.BeginAction();
				for ( const auto [entity, type] : inRegistry.view<const T>().each() ) {
					if ( inType == type ) inSelectionContext.AddSelectedEntity( entity );
				}
				inEntityManager.EndAction( inRegistry );
			};
			if ( ImGui::Selectable( "Remove children from selection" ) ) {
				inEntityManager.BeginAction();
				for ( const auto [entity, type] : inRegistry.view<const T>().each() ) {
					if ( inType == type ) inSelectionContext.DeselectEntity( entity );
				}
				inEntityManager.EndAction( inRegistry );
			};
			ImGui::Separator();
			if ( ImGui::Selectable( "Set all children Visible" ) ) UpdateBoolPerPredicate<Visible>( inRegistry, inEntityManager, inType, true );
			if ( ImGui::Selectable( "Set all children Hidden" ) ) UpdateBoolPerPredicate<Visible>( inRegistry, inEntityManager, inType, false );
			ImGui::Separator();
			if ( ImGui::Selectable( "Set all children Selectable" ) ) UpdateBoolPerPredicate<Selectable>( inRegistry, inEntityManager, inType, true );
			if ( ImGui::Selectable( "Set all children Unselectable" ) ) UpdateBoolPerPredicate<Selectable>( inRegistry, inEntityManager, inType, false );
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar( 2 );
	}

	void EntityPopup( Cyclone::Core::EntityManager &inEntityManager, entt::entity inEntity )
	{
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
		if ( ImGui::BeginPopupContextItem( "Entity Popup" ) ) {
			if ( ImGui::Selectable( "Open Properties" ) ) {
				inEntityManager.OpenEntityProperties( inEntity );
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar( 2 );
	}

	ImGuiChildFlags cSectionChildFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY;
	ImGuiWindowFlags cSectionWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
	ImGuiTableFlags cTableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_NoBordersInBody;
	ImGuiTreeNodeFlags cTreeNodeFlags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_FramePadding;
}

void Cyclone::UI::Outliner::Update( Cyclone::Core::LevelInterface *inLevelInterface )
{
	auto &entityManager = inLevelInterface->GetEntityManager();

	ImGui::BeginDisabled( !entityManager.CanAquireActionLock() );
	{
		if ( ImGui::CollapsingHeader( "Outliner", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			OutlinerTreeUpdate( inLevelInterface );
		}

		if ( ImGui::CollapsingHeader( "Selection", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			SelectionListUpdate( inLevelInterface );
		}

		if ( ImGui::CollapsingHeader( "Undo History", ImGuiTreeNodeFlags_DefaultOpen ) ) {
			UndoHistoryUpdate( inLevelInterface );
		}
	}
	ImGui::EndDisabled();

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
	auto view = cregistry.group_if_exists<EntityType, EntityCategory, Visible, Selectable>();
	for ( const entt::entity entity : view ) {
		const auto &entityType = view.get<EntityType>( entity );
		const auto &entityCategory = view.get<EntityCategory>( entity );

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

	for ( auto &[entityCategory, typeMap] : mOutlinerTree ) {
		for ( auto &[entityType, entityList] : typeMap ) {
			std::sort( std::execution::par_unseq, entityList.begin(), entityList.end() );
		}
	}
}

void Cyclone::UI::Outliner::OutlinerTreeUpdate( Cyclone::Core::LevelInterface *inLevelInterface )
{
	RebuildTree( inLevelInterface );

	auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &entityManager = inLevelInterface->GetEntityManager();
	entt::registry &registry = inLevelInterface->GetRegistry();

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 32.0f }, { ImGui::GetContentRegionAvail().x, mOutlinerHeight + mRemainingHeight } );
	if ( ImGui::BeginChild( "OutlinerChild", { 0.0f, 256.0f }, cSectionChildFlags, cSectionWindowFlags ) ) {
		if ( ImGui::BeginTable( "OutlinerTable", 3, cTableFlags, { 0.0f, -1.0f } ) ) {

			ImGui::TableSetupColumn( "Name" );
			ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
			ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
			ImGui::TableSetupScrollFreeze( 0, 1 );
			ImGui::TableHeadersRow();

			ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, { 0.0f, 0.0f } );

			for ( const auto &[entityCategory, typeMap] : mOutlinerTree ) {
				bool categoryVisible = entityManager.GetEntityCategoryIsVisible( entityCategory );
				bool categorySelectable = entityManager.GetEntityCategoryIsSelectable( entityCategory );

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex( 1 );
				if ( DrawTreeNodeCheckbox( style, Cyclone::Util::PrefixString( "##cV", entityCategory ), categoryVisible ) ) entityManager.SetEntityCategoryIsVisible( registry, entityCategory, categoryVisible ^= true );

				ImGui::TableSetColumnIndex( 2 );
				if ( DrawTreeNodeCheckbox( style, Cyclone::Util::PrefixString( "##cS", entityCategory ), categorySelectable ) ) entityManager.SetEntityCategoryIsSelectable( registry, entityCategory, categorySelectable ^= true );

				ImGui::TableSetColumnIndex( 0 );
				bool entityCategoryNodeOpen = ImGui::TreeNodeEx( entityManager.GetEntityCategoryName( entityCategory ), cTreeNodeFlags );

				TreePopup( registry, entityManager, selectionContext, entityCategory );

				if ( entityCategoryNodeOpen ) {
					for ( const auto &[entityType, entityList] : typeMap ) {
						if ( entityList.empty() ) continue;

						ImGui::TableNextRow();

						bool entityTypeVisible = entityManager.GetEntityTypeIsVisible( entityType );
						bool entityTypeSelectable = entityManager.GetEntityTypeIsSelectable( entityType );

						ImGui::TableSetColumnIndex( 1 );
						if ( DrawTreeNodeCheckbox( style, Cyclone::Util::PrefixString( "##tV", entityType ), entityTypeVisible ) ) entityManager.SetEntityTypeIsVisible( registry, entityType, entityTypeVisible ^= true );

						ImGui::TableSetColumnIndex( 2 );
						if ( DrawTreeNodeCheckbox( style, Cyclone::Util::PrefixString( "##tS", entityType ), entityTypeSelectable ) ) entityManager.SetEntityTypeIsSelectable( registry, entityType, entityTypeSelectable ^= true );

						ImGui::TableSetColumnIndex( 0 );
						bool entityTypeNodeOpen = ImGui::TreeNodeEx( entityManager.GetEntityTypeName( entityType ), cTreeNodeFlags );

						TreePopup( registry, entityManager, selectionContext, entityType );

						if ( entityTypeNodeOpen ) {

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

									auto &entityVisible = registry.get<Visible>( entity );
									auto &entitySelectable = registry.get<Selectable>( entity );

									bool rowVisible = static_cast<bool>( entityVisible ) && categoryVisible && entityTypeVisible;
									bool rowSelectable = static_cast<bool>( entitySelectable ) && categorySelectable && entityTypeSelectable;

									if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;
									if ( !rowVisible ) selectionFlags |= ImGuiSelectableFlags_Disabled;
									if ( !rowSelectable ) selectionFlags |= ImGuiSelectableFlags_Disabled;

									if ( !( selectionFlags & ImGuiSelectableFlags_Disabled ) ) treeLeafFlags |= ImGuiTreeNodeFlags_Bullet;

									const auto entityIdString = Cyclone::Util::PrefixString( "##b", entity );
									ImGui::TreeNodeEx( entityIdString, treeLeafFlags | ImGuiTreeNodeFlags_FramePadding );

									//ImGui::Bullet();
									ImGui::SameLine( 0, 0 );
									ImGui::SetCursorPosY( ImGui::GetCursorPosY() - style.FramePadding.y );
									ImGui::SetNextItemAllowOverlap();
									ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.0f, 0.5f } );
									if ( ImGui::Selectable( entityIdString.Value(), entityInSelection, selectionFlags, { 0, style.FramePadding.y * 2 + ImGui::GetTextLineHeight() } ) ) {
										HandleEntityClick( registry, entityManager, selectionContext, io, entity, entityIsSelected );
									}
									ImGui::PopStyleVar( 1 );

									EntityPopup( entityManager, entity );

									ImGui::TableSetColumnIndex( 1 );
									if ( DrawTreeNodeCheckbox( style, "##V", static_cast<bool>( entityVisible ) ) ) UpdateBoolPerEntity( registry, entityManager, entity, entityVisible );

									ImGui::TableSetColumnIndex( 2 );
									if ( DrawTreeNodeCheckbox( style, "##S", static_cast<bool>( entitySelectable ) ) ) UpdateBoolPerEntity( registry, entityManager, entity, entitySelectable );

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

void Cyclone::UI::Outliner::SelectionListUpdate( Cyclone::Core::LevelInterface *inLevelInterface )
{
	auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &entityManager = inLevelInterface->GetEntityManager();
	entt::registry &registry = inLevelInterface->GetRegistry();

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	auto view = registry.view<EntityType, Visible, Selectable>();
	ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 32.0f }, { ImGui::GetContentRegionAvail().x, mSelectionHeight + mRemainingHeight } );
	if ( ImGui::BeginChild( "SelectionChild", { 0.0f, 256.0f }, cSectionChildFlags, cSectionWindowFlags ) ) {
		if ( ImGui::BeginTable( "SelectionTable", 4, cTableFlags, { 0.0f, -1.0f } ) ) {
			ImGui::TableSetupColumn( "Type" );
			ImGui::TableSetupColumn( "Name" );
			ImGui::TableSetupColumn( "V", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
			ImGui::TableSetupColumn( "S", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight() );
			ImGui::TableSetupScrollFreeze( 0, 1 );
			ImGui::TableHeadersRow();

			ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, { 0.0f, 0.0f } );

			// Explicitly create copy rather than ref
			std::vector<entt::entity> previousSelection;
			previousSelection.reserve( selectionContext.GetSelectedEntities().size() );
			std::copy_if( selectionContext.GetSelectedEntities().begin(), selectionContext.GetSelectedEntities().end(), std::back_inserter( previousSelection ), []( auto i ){ return entt::to_version( i ) == 0; } );

			ImGuiListClipper clipper;
			clipper.Begin( static_cast<int>( previousSelection.size() ) );

			while ( clipper.Step() ) {
				for ( int rowN = clipper.DisplayStart; rowN < clipper.DisplayEnd; ++rowN ) {
					entt::entity entity = previousSelection[rowN];

					ImGui::PushID( static_cast<int>( entity ) );

					bool entityIsSelected = selectionContext.GetSelectedEntity() == entity;

					ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
					if ( entityIsSelected ) selectionFlags |= ImGuiSelectableFlags_Highlight;

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex( 0 );

					const auto &entityType = view.get<EntityType>( entity );
					ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.0f, 0.5f } );
					ImGui::SetNextItemAllowOverlap();
					if ( ImGui::Selectable( entityManager.GetEntityTypeName( entityType ), true, selectionFlags, { 0, style.FramePadding.y * 2 + ImGui::GetTextLineHeight() } ) ) {
						HandleEntityClick( registry, entityManager, selectionContext, io, entity, entityIsSelected );
					}
					ImGui::PopStyleVar( 1 );

					EntityPopup( entityManager, entity );

					ImGui::TableSetColumnIndex( 1 );
					ImGui::AlignTextToFramePadding();
					ImGui::Text( Cyclone::Util::PrefixString( "", entity ) );

					auto &entityVisible = view.get<Visible>( entity );
					auto &entitySelectable = view.get<Selectable>( entity );

					ImGui::TableSetColumnIndex( 2 );
					if ( DrawTreeNodeCheckbox( style, "##V", static_cast<bool>( entityVisible ) ) ) UpdateBoolPerEntity( registry, entityManager, entity, entityVisible );

					ImGui::TableSetColumnIndex( 3 );
					if ( DrawTreeNodeCheckbox( style, "##S", static_cast<bool>( entitySelectable ) ) ) UpdateBoolPerEntity( registry, entityManager, entity, entitySelectable );

					ImGui::PopID();
				}
			}

			ImGui::PopStyleVar( 1 );

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
	mSelectionHeight = ImGui::GetItemRectSize().y;
}

void Cyclone::UI::Outliner::UndoHistoryUpdate( Cyclone::Core::LevelInterface *inLevelInterface )
{
	auto &entityManager = inLevelInterface->GetEntityManager();
	entt::registry &registry = inLevelInterface->GetRegistry();

	ImGuiStyle &style = ImGui::GetStyle();

	auto view = registry.view<EntityType, Visible, Selectable>();
	ImGui::SetNextWindowSizeConstraints( { ImGui::GetContentRegionAvail().x, 32.0f }, { ImGui::GetContentRegionAvail().x, mUndoHistoryHeight + mRemainingHeight } );
	if ( ImGui::BeginChild( "UndoHistoryChild", { 0.0f, 256.0f }, cSectionChildFlags, cSectionWindowFlags ) ) {
		if ( ImGui::BeginTable( "UndoHistoryTable", 4, cTableFlags, { 0.0f, -1.0f } ) ) {

			ImGui::TableSetupColumn( "Epoch" );
			ImGui::TableSetupColumn( "Total" );
			ImGui::TableSetupColumn( "Created" );
			ImGui::TableSetupColumn( "Updated" );
			ImGui::TableSetupScrollFreeze( 0, 1 );
			ImGui::TableHeadersRow();

			const auto &undoStack = entityManager.GetUndoStack();
			const size_t currentEpoch = entityManager.GetUndoEpoch();
			size_t chosenEpoch = currentEpoch;

			ImGuiListClipper clipper;
			clipper.Begin( static_cast<int>( undoStack.size() ) );

			while ( clipper.Step() ) {
				for ( int rowN = clipper.DisplayStart; rowN < clipper.DisplayEnd; ++rowN ) {
					//for ( int epoch = static_cast<int>( undoStack.size() ) - 1; epoch >= 0; --epoch ) {
					int epoch = static_cast<int>( undoStack.size() ) - 1 - rowN;

					ImGui::PushID( epoch );

					const entt::registry &epochRegistry = undoStack[epoch].mRegistry;

					size_t nChanges = epochRegistry.view<entt::entity>().size();
					size_t nUpdates = epochRegistry.view<EpochNumber>().size();

					bool isCurrent = epoch == currentEpoch;
					bool disabled = epoch > currentEpoch;

					if ( disabled ) ImGui::PushStyleColor( ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled] );

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
			}

			if ( chosenEpoch != currentEpoch ) {
				while ( entityManager.GetUndoEpoch() > chosenEpoch ) {
					entityManager.UndoAction( registry );
				}

				while ( entityManager.GetUndoEpoch() < chosenEpoch ) {
					entityManager.RedoAction( registry );
				}
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
	mUndoHistoryHeight = ImGui::GetItemRectSize().y;
}
