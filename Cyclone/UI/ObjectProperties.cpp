#include "pch.h"
#include "Cyclone/UI/ObjectProperties.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/Rotation.hpp"

using Cyclone::Math::Vector4D;
using namespace Cyclone::Core::Component;

void Cyclone::UI::ObjectProperties::ShowWindow( Cyclone::Core::LevelInterface *inLevelInterface, entt::entity inEntity )
{
	entt::registry &registry = inLevelInterface->GetRegistry();
	auto &entityManager = inLevelInterface->GetEntityManager();

	bool dirty = false;

	Position &position = registry.get<Position>( inEntity );
	double positionData[4];
	position.mValue.Store( positionData );
	ImGui::Text( "Position" );
	ImGui::SameLine( 128.0f );
	ImGui::InputScalarN( "##Position", ImGuiDataType_Double, positionData, 3, nullptr, nullptr, "%.2f" );
	if ( ImGui::IsItemDeactivatedAfterEdit() ) {
		position.mValue = Vector4D::sLoad( positionData ); // TODO: snapping?
		dirty = true;
	}

	Rotation &rotation = registry.get<Rotation>( inEntity );
	DirectX::XMVECTORF32 rotationData = { .v = rotation.mPitchYawRoll };
	rotationData.v = rotationData * ( 180.0f / DirectX::XM_PI );
	ImGui::Text( "Rotation" );
	ImGui::SameLine( 128.0f );
	ImGui::InputScalarN( "##Rotation", ImGuiDataType_Float, rotationData.f, 3, nullptr, nullptr, "%.2f" );
	if ( ImGui::IsItemDeactivatedAfterEdit() ) {
		rotation.mPitchYawRoll = rotationData * ( DirectX::XM_PI / 180.0f );
		dirty = true;

		// TODO: update bounding box
	}

	if ( dirty ) {
		entityManager.BeginAction();
		entityManager.UpdateEntity( inEntity, registry );
		entityManager.EndAction( registry );
	}
}
