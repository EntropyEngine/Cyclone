#include "pch.h"
#include "Cyclone/UI/Tool/SelectionTool.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// Cyclone math
#include "Cyclone/Math/Vector.hpp"

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::SelectionTool::OnClick( Cyclone::Core::LevelInterface *inLevelInterface, double inWorldSpaceU, double inWorldSpaceV, double inHandleRadius, double inWorldLimit )
{
	bool ctrlHeld = ImGui::IsKeyDown( ImGuiMod_Ctrl );
	bool shiftHeld = ImGui::IsKeyDown( ImGuiMod_Shift );

	if ( !( ctrlHeld || shiftHeld ) ) inLevelInterface->ClearSelection();

	// Not a reference; need original
	const std::set<entt::entity> previousSelected = inLevelInterface->GetSelectedEntities();

	double clickPositionD[4] = { 0, 0, 0, 0 };
	clickPositionD[ViewportTypeTraits<T>::AxisU] = inWorldSpaceU;
	clickPositionD[ViewportTypeTraits<T>::AxisV] = inWorldSpaceV;
	Cyclone::Math::Vector4D clickPosition = Cyclone::Math::Vector4D::sLoad( clickPositionD );
	Cyclone::Math::Vector4D clickExtent = Cyclone::Math::Vector4D::sZeroSetValueByIndex<ViewportTypeTraits<T>::AxisW>( inWorldLimit );
	Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> clickBox{ .mCenter = clickPosition, .mExtent = clickExtent };

	Cyclone::Math::Vector4D entityExtent = Cyclone::Math::Vector4D::sReplicate( inHandleRadius );

	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	auto view = cregistry.view<Cyclone::Core::Component::Position>();
	for ( const entt::entity entity : view ) {
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
		Cyclone::Math::BoundingBox<Cyclone::Math::Vector4D> enitityHandle{ .mCenter = position, .mExtent = entityExtent };

		if ( enitityHandle.Intersects( clickBox ) ) {

			if ( ctrlHeld ) {
				if ( shiftHeld ) inLevelInterface->AddSelectedEntity( entity );
				else if ( previousSelected.contains( entity ) ) {
					inLevelInterface->DeselectEntity( entity );
					break;
				}
			}
			else if ( shiftHeld ) {
				inLevelInterface->AddSelectedEntity( entity );
			}
			else {
				inLevelInterface->AddSelectedEntity( entity );
				break;
			}
		}
	}
}


template void Cyclone::UI::Tool::SelectionTool::OnClick<Cyclone::UI::EViewportType::TopXZ>( Cyclone::Core::LevelInterface *, double, double, double, double );
template void Cyclone::UI::Tool::SelectionTool::OnClick<Cyclone::UI::EViewportType::FrontXY>( Cyclone::Core::LevelInterface *, double, double, double, double );
template void Cyclone::UI::Tool::SelectionTool::OnClick<Cyclone::UI::EViewportType::SideYZ>( Cyclone::Core::LevelInterface *, double, double, double, double );
