#include "pch.h"

#include "Cyclone/UI/MainUI.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui_internal.h>

Cyclone::UI::MainUI::MainUI() noexcept :
	mVerticalSyncEnabled( true )
{}

void Cyclone::UI::MainUI::Initialize()
{
	
}

void Cyclone::UI::MainUI::Update( float inDeltaTime )
{
	if ( ImGui::BeginMainMenuBar() ) {
		if ( ImGui::BeginMenu( "File" ) ) {
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking;
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos( viewport->WorkPos );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x - 256, 64 } );
	if ( ImGui::Begin( "ToolBar", nullptr, windowFlags ) ) {

	}
	ImGui::End();

	ImGui::SetNextWindowPos( { viewport->WorkPos.x, viewport->WorkPos.y + 64 } );
	ImGui::SetNextWindowSize( { 64, viewport->WorkSize.y - 64 } );
	if ( ImGui::Begin( "SideBar", nullptr, windowFlags ) ) {

	}
	ImGui::End();

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + 64, viewport->WorkPos.y + 64 }  );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x - 64 - 256, viewport->WorkSize.y - 64 } );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f } );
	if ( ImGui::Begin( "MainWindow", nullptr, windowFlags ) ) {

		float consumedHeight = 0.0f;
		float consumedWidth = 0.0f;

		ImGui::SetNextWindowSizeConstraints( { 64.0f, 64.0f }, { ImGui::GetContentRegionAvail().x - 64.0f, ImGui::GetContentRegionAvail().y - 64.0f } );
		if ( ImGui::BeginChild( "PerspectiveView", ImVec2( ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
			ImGui::Text( "Yeet" );
			consumedHeight = ImGui::GetWindowHeight();
			consumedWidth = ImGui::GetWindowWidth();
			ImGui::EndChild();
		}

		ImGui::SameLine();

		if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, consumedHeight ), ImGuiChildFlags_Borders, viewportFlags ) ) {
			ImGui::Text( "Yeet" );
			ImGui::EndChild();
		}

		if ( ImGui::BeginChild( "FrontView", ImVec2( consumedWidth, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
			ImGui::Text( "Yeet" );
			ImGui::EndChild();
		}

		ImGui::SameLine();

		if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
			ImGui::Text( "Yeet" );
			ImGui::EndChild();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar( 2 );

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + viewport->WorkSize.x - 256, viewport->WorkPos.y }  );
	ImGui::SetNextWindowSize( { 256, viewport->WorkSize.y } );
	if ( ImGui::Begin( "Outliner", nullptr, windowFlags ) ) {

	}
	ImGui::End();

	//ImGui::PopStyleVar();
	
}