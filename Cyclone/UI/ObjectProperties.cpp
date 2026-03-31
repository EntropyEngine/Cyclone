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

	ImGui::Text( "Type" );
	ImGui::SameLine( 128.0f );
	ImGui::Text( entityManager.GetEntityTypeName( entityType ) );

	ImGui::Text( "Category" );
	ImGui::SameLine( 128.0f );
	ImGui::Text( entityManager.GetEntityCategoryName( entityCategory ) );

	ImGui::Separator();

	{
		Position &position = registry.get<Position>( inEntity );
		double positionData[4];
		position.mValue.Store( positionData );
		ImGui::Text( "Position" );
		ImGui::SameLine( 128.0f );
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
		ImGui::Text( "Rotation" );
		ImGui::SameLine( 128.0f );
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

		ImGui::Text( "Segment Count" );
		ImGui::SameLine( 128.0f );
		ImGui::Text( Cyclone::Util::PrefixString( "", pathData.mPathSegments.size() ) );
	}


	if ( dirty ) {
		entityManager.BeginAction();
		entityManager.UpdateEntity( inEntity, registry );
		entityManager.EndAction( registry );
	}
}
