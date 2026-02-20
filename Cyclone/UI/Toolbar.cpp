#include "pch.h"
#include "Cyclone/UI/Toolbar.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_internal.h>

void Cyclone::UI::Toolbar::Update( Cyclone::Core::LevelInterface *inLevelInterface )
{
	auto &gridContext = inLevelInterface->GetGridCtx();

	ImVec2 viewSize = ImGui::GetWindowSize();

	
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 4.0f );

	const float textOffset = 6.0f;
	const float itemOffset = 3.0f;

	const float textSpacing = 5.0f;
	const float childSpacing = 8.0f;

	ImGui::SetCursorPos( { 5, 5 } );

	ImGui::BeginChild( "##GridSizeChunk", { 0, viewSize.y - 10 }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_FrameStyle );
	{
		ImGui::SetCursorPosY( textOffset );
		ImGui::Text( "Grid:" );

		ImGui::SameLine( 0.0f, textSpacing );

		ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.5f } );
		ImGui::SetCursorPosY( itemOffset );
		ImGui::SetNextItemWidth( 52.0f );
		if ( ImGui::BeginCombo( "##SubGridLevel", gridContext.kGridSizeText[gridContext.mGridSizeIndex], ImGuiComboFlags_HeightLarge ) ) {
			ImGui::Separator();
			for ( int i = 0; i < std::size( gridContext.kGridSizes ); ++i ) {
				const bool isSelected = gridContext.mGridSizeIndex == i;
				if ( ImGui::Selectable( gridContext.kGridSizeText[i], isSelected ) ) {
					gridContext.SetGridSize( i );
				}
				if ( isSelected ) ImGui::SetItemDefaultFocus();
			}

			ImGui::Separator();
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar( 1 );

		// Adjust grid size with [ and ]
		if ( ImGui::IsKeyPressed( ImGuiKey_LeftBracket, false ) ) gridContext.SetGridSize( gridContext.mGridSizeIndex - 1 );
		if ( ImGui::IsKeyPressed( ImGuiKey_RightBracket, false ) ) gridContext.SetGridSize( gridContext.mGridSizeIndex + 1 );
	}
	ImGui::EndChild();

	ImGui::SameLine( 0.0f, childSpacing );

	ImGui::BeginChild( "##GridSnapChunk", { 0, viewSize.y - 10 }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_FrameStyle );
	{
		ImGui::SetCursorPosY( textOffset );
		ImGui::Text( "Snap:" );

		ImGui::SameLine( 0.0f, textSpacing );

		ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, { 0.5f, 0.5f } );
		ImGui::SetCursorPosY( itemOffset );
		ImGui::SetNextItemWidth( 80.0f );
		if ( ImGui::BeginCombo( "##GridSnapType", gridContext.kSnapTypeText[static_cast<int>( gridContext.mSnapType )], ImGuiComboFlags_HeightLarge ) ) {
			ImGui::Separator();
			for ( int i = 0; i < std::size( gridContext.kSnapTypeText ); ++i ) {
				const bool isSelected = static_cast<int>( gridContext.mSnapType ) == i;
				if ( ImGui::Selectable( gridContext.kSnapTypeText[i], isSelected ) ) {
					gridContext.mSnapType = static_cast<Cyclone::Core::Editor::GridContext::ESnapType>( i );
				}
				if ( isSelected ) ImGui::SetItemDefaultFocus();
			}
			ImGui::Separator();
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar( 1 );

	}
	ImGui::EndChild();

	ImGui::PopStyleVar( 1 );
}