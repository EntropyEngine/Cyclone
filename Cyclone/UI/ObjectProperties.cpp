#include "pch.h"
#include "Cyclone/UI/ObjectProperties.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/LocalBounds.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone utils
#include "Cyclone/Util/String.hpp"

using Cyclone::Math::Vector4D;
using namespace Cyclone::Core::Component;

void Cyclone::UI::ObjectProperties::ShowWindow( Cyclone::Core::LevelInterface *inLevelInterface, entt::entity inEntity )
{
	entt::registry &registry = inLevelInterface->GetRegistry();
	auto &entityManager = inLevelInterface->GetEntityManager();

	bool dirty = false;

	EntityType entityType = registry.get<EntityType>( inEntity );
	EntityCategory entityCategory = registry.get<EntityCategory>( inEntity );

	ImGui::AlignTextToFramePadding();
	ImGui::Text( "Type" );
	ImGui::SameLine( 128.0f );
	ImGui::Text( entityManager.GetEntityTypeName( entityType ) );

	ImGui::AlignTextToFramePadding();
	ImGui::Text( "Category" );
	ImGui::SameLine( 128.0f );
	ImGui::Text( entityManager.GetEntityCategoryName( entityCategory ) );

	ImGui::Separator();

	{
		Position &position = registry.get<Position>( inEntity );
		double positionData[4];
		position.mValue.Store( positionData );
		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Position" );
		ImGui::SameLine( 128.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::DragScalarN( "##Position", ImGuiDataType_Double, positionData, 3, 1.0f, nullptr, nullptr, "%.2f" );
		if ( ImGui::IsItemEdited() ) {
			position.mValue = Vector4D::sLoad( positionData ); // TODO: snapping?
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() ) {
			dirty = true;
		}
	}

	{
		Rotation &rotation = registry.get<Rotation>( inEntity );
		DirectX::XMVECTORF32 rotationData = { .v = rotation.mPitchYawRoll };
		rotationData.v = rotationData * ( 180.0f / DirectX::XM_PI );
		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Rotation" );
		ImGui::SameLine( 128.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::DragScalarN( "##Rotation", ImGuiDataType_Float, rotationData.f, 3, 1.0f, nullptr, nullptr, "%.2f" );
		if ( ImGui::IsItemEdited() ) {
			rotation.mPitchYawRoll = rotationData * ( DirectX::XM_PI / 180.0f );
			registry.get<LocalBounds>( inEntity ).UpdateBoundingBox( inEntity, registry );
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() ) {
			dirty = true;
		}
	}

	if ( registry.all_of<PathTag>( inEntity ) ) {
		ImGui::SeparatorText( "Path Data" );

		PathData &pathData = registry.get<PathData>( inEntity );

		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Knot Count" );
		ImGui::SameLine( 128.0f );
		ImGui::Text( Cyclone::Util::PrefixString( "", pathData.mKnots.size() ) );

		for ( int i = 0; i < pathData.mKnots.size(); ++i ) {
			ImGui::AlignTextToFramePadding();
			if ( ImGui::TreeNode( Cyclone::Util::PrefixString( "Knot ", i ) ) ) {

				{
					Vector4D &position = pathData.mKnots[i].mPoint;
					double positionData[4];
					position.Store( positionData );

					ImGui::AlignTextToFramePadding();
					ImGui::Text( "Rel Position" );
					ImGui::SameLine( 128.0f );
					ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
					ImGui::DragScalarN( "##RelPosition", ImGuiDataType_Double, positionData, 3, 1.0f, nullptr, nullptr, "%.2f" );

					if ( ImGui::IsItemEdited() ) {
						position = Vector4D::sLoad( positionData );
					}
					if ( ImGui::IsItemDeactivatedAfterEdit() ) {
						dirty = true;
					}
				}

				{
					ImGui::AlignTextToFramePadding();
					ImGui::Text( "Align Normal" );
					ImGui::SameLine( 128.0f );

					int nidx = ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::NORMAL_MASK ) - 1;
					ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
					if ( ImGui::Combo( "##N", &nidx, "Explict\0Aligned\0Tilt" ) ) {
						pathData.mExtrusionTypes[i] &= ~PathData::EExtrusionType::NORMAL_MASK;
						pathData.mExtrusionTypes[i] |= nidx + 1;
						dirty = true;
					}
				}

				{
					ImGui::AlignTextToFramePadding();
					ImGui::Text( "Align Bitan" );
					ImGui::SameLine( 128.0f );

					int bidx = ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::BITANGENT_MASK ) >> 2 ) - 1;
					ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
					if ( ImGui::Combo( "##B", &bidx, "Explict\0Aligned\0Tilt" ) ) {
						pathData.mExtrusionTypes[i] &= ~PathData::EExtrusionType::BITANGENT_MASK;
						pathData.mExtrusionTypes[i] |= ( bidx + 1 ) << 2;
						dirty = true;
					}
				}

				ImGui::TreePop();
			}
		}
	}


	if ( dirty ) {
		entityManager.BeginAction();
		entityManager.UpdateEntity( inEntity, registry );
		entityManager.EndAction( registry );
	}
}
