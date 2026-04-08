#include "pch.h"
#include "Cyclone/UI/Sidebar.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone UI tools
#include "Cyclone/UI/Tool/SelectionTool.hpp"
#include "Cyclone/UI/Tool/SelectionTransformTool.hpp"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"

#include "Cyclone/UI/Tool/PathActiveTool.hpp"
#include "Cyclone/UI/Tool/PathSelectionTool.hpp"

#include "Cyclone/UI/Tool/MeshSelectionTool.hpp"

void Cyclone::UI::Sidebar::Init()
{
	mToolChanger.emplace_back( std::make_unique<Tool::SelectionTool>() );
	mToolChanger.emplace_back( std::make_unique<Tool::SelectionTransformTool>() );
	mToolChanger.emplace_back( std::make_unique<Tool::GizmoTransformTool>() );

	mToolChanger.emplace_back( std::make_unique<Tool::PathActiveTool>() );
	mToolChanger.emplace_back( std::make_unique<Tool::PathSelectionTool>() );

	mToolChanger.emplace_back( std::make_unique<Tool::MeshSelectionTool>() );

	mCurrentCategory = Tool::ECategory::Object;

	mToolChanger[0]->mIsSelected = true;
	mToolChanger[1]->mIsSelected = true;

	for ( auto &tool : mToolChanger ) {
		mToolCategories[static_cast<size_t>( tool->GetCategory() )].push_back( tool.get() );
	}
}

void Cyclone::UI::Sidebar::Update( Cyclone::Core::LevelInterface *inLevelInterface )
{
	float buttonSize = ImGui::GetContentRegionAvail().x;

	ImGui::BeginDisabled( !inLevelInterface->GetEntityManager().CanAquireActionLock() );

	ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.5f } );

	for ( auto &category : mToolCategories ) {
		ImGui::Separator();
		ImGui::Dummy( {} );
		for ( auto tool : category ) {
			if ( ImGui::Selectable( tool->GetDebugName(), true, tool->mIsSelected ? ImGuiSelectableFlags_Highlight : 0, { buttonSize, buttonSize } ) ) {
				SelectTool( tool );
			}
			ImGui::Dummy( {} );
		}
	}

	ImGui::PopStyleVar();

	ImGui::EndDisabled();
}

void Cyclone::UI::Sidebar::SelectTool( Tool::BaseTool *inTool )
{
	for ( auto &tool : mToolChanger ) {
		if ( tool->GetCategory() != inTool->GetCategory() ) tool->mIsSelected = false;
	}

	mCurrentCategory = inTool->GetCategory();
	auto &category = mToolCategories[static_cast<size_t>( inTool->GetCategory() )];

	switch ( inTool->GetSelectMode() ) {
		case Tool::ESelectMode::ToggleInCategory: {
			for ( auto &tool : category ) {
				if ( tool->GetSelectMode() == Tool::ESelectMode::ActiveInCategory ) tool->mIsSelected = true;
				if ( tool->GetSelectMode() == Tool::ESelectMode::UniqueInCategory ) tool->mIsSelected = false;
			}
			inTool->mIsSelected ^= true;
			break;
		}
		case Tool::ESelectMode::SelectInCategory: {
			for ( auto &tool : category ) {
				if ( tool->GetSelectMode() == Tool::ESelectMode::ActiveInCategory ) tool->mIsSelected = true;
				else if ( tool->GetSelectMode() != Tool::ESelectMode::ToggleInCategory ) tool->mIsSelected = false;
			}
			inTool->mIsSelected = true;
			break;
		}
		case Tool::ESelectMode::UniqueInCategory: {
			for ( auto &tool : category ) {
				if ( tool->GetSelectMode() == Tool::ESelectMode::ActiveInCategory ) tool->mIsSelected = true;
				else																tool->mIsSelected = false;
			}
			inTool->mIsSelected = true;
			break;
		}	
		case Tool::ESelectMode::ActiveInCategory: {
			inTool->mIsSelected = true;
			break;
		}
	}
}
