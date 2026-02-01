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

void Cyclone::UI::MainUI::Initialize( ID3D11Device3 *inDevice )
{
	mViewportManager = std::make_unique<Cyclone::UI::ViewportManager>( inDevice );
}

void Cyclone::UI::MainUI::Update( float inDeltaTime )
{
	if ( ImGui::BeginMainMenuBar() ) {
		if ( ImGui::BeginMenu( "File" ) ) {
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;

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

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + 64, viewport->WorkPos.y + 64 } );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x - 64 - 256, viewport->WorkSize.y - 64 } );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f } );
	if ( ImGui::Begin( "MainWindow", nullptr, windowFlags | ImGuiWindowFlags_NoDecoration ) ) {
		mViewportManager->Update( inDeltaTime );
	}
	ImGui::End();
	ImGui::PopStyleVar( 2 );

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + viewport->WorkSize.x - 256, viewport->WorkPos.y } );
	ImGui::SetNextWindowSize( { 256, viewport->WorkSize.y } );
	if ( ImGui::Begin( "Outliner", nullptr, windowFlags ) ) {

	}
	ImGui::End();

	//ImGui::PopStyleVar();

}

void Cyclone::UI::MainUI::Render( ID3D11DeviceContext3 *inDeviceContext )
{
	mViewportManager->RenderPerspective( inDeviceContext );
	mViewportManager->RenderTop( inDeviceContext );
	mViewportManager->RenderFront( inDeviceContext );
	mViewportManager->RenderSide( inDeviceContext );
}
