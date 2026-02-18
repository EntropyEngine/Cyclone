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
	if ( mSelectedEntities.erase( inEntity ) ) {
		if ( inEntity == mSelectedEntity || true ) {
			if ( mSelectedEntities.empty() ) mSelectedEntity = entt::null;
			else {
				auto it = mSelectedEntities.upper_bound( inEntity );
				if ( it == mSelectedEntities.end() ) {
					it = mSelectedEntities.lower_bound( inEntity );
				}
				if ( it == mSelectedEntities.end() ) {
					it = mSelectedEntities.begin();
				}
				mSelectedEntity = *it;
			}
		}
	}
}

void Cyclone::Core::Tool::SelectionTool::ClearSelection()
{
	mSelectedEntities.clear();
	mSelectedEntity = entt::null;
}