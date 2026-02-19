#include "pch.h"
#include "Cyclone/Core/Tool/SelectionToolContext.hpp"

void Cyclone::Core::Tool::SelectionToolContext::SetSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.clear();
	mSelectedEntities.insert( inEntity );
}

void Cyclone::Core::Tool::SelectionToolContext::AddSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.insert( inEntity );
}

void Cyclone::Core::Tool::SelectionToolContext::DeselectEntity( entt::entity inEntity )
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

void Cyclone::Core::Tool::SelectionToolContext::ClearSelection()
{
	mSelectedEntities.clear();
	mSelectedEntity = entt::null;
}