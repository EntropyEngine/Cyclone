#include "pch.h"
#include "Cyclone/Core/Tool/SelectionTool.hpp"

void Cyclone::Core::Tool::SelectionTool::SetSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.clear();
	mSelectedEntities.insert( inEntity );
}

void Cyclone::Core::Tool::SelectionTool::AddSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.insert( inEntity );
}

void Cyclone::Core::Tool::SelectionTool::DeselectEntity( entt::entity inEntity )
{
	if ( inEntity == mSelectedEntity ) mSelectedEntity = entt::null;
	mSelectedEntities.erase( inEntity );

	if ( mSelectedEntities.size() > 0 ) mSelectedEntity = *mSelectedEntities.begin();
}

void Cyclone::Core::Tool::SelectionTool::ClearSelection()
{
	mSelectedEntities.clear();
	mSelectedEntity = entt::null;
}