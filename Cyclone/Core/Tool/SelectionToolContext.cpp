#include "pch.h"
#include "Cyclone/Core/Tool/SelectionToolContext.hpp"

void Cyclone::Core::Tool::SelectionToolContext::SetSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.clear();
	mSelectedEntities.insert( inEntity );
	mDirty = true;
}

void Cyclone::Core::Tool::SelectionToolContext::AddSelectedEntity( entt::entity inEntity )
{
	mSelectedEntity = inEntity;
	mSelectedEntities.insert( inEntity );
	mDirty = true;
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
		mDirty = true;
	}
}

void Cyclone::Core::Tool::SelectionToolContext::ClearSelection()
{
	if ( mSelectedEntities.size() ) {
		mSelectedEntities.clear();
		mSelectedEntity = entt::null;
		mDirty = true;
	}
}