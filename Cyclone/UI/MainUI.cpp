#include "pch.h"
#include "Cyclone/UI/MainUI.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Selectable.hpp"

// Cyclone UI includes
#include "Cyclone/UI/ViewportManager.hpp"
#include "Cyclone/UI/Outliner.hpp"
#include "Cyclone/UI/Toolbar.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_internal.h>

// STL
#include <format>

Cyclone::UI::MainUI::MainUI() noexcept :
	mVerticalSyncEnabled( true )
{}

Cyclone::UI::MainUI::~MainUI()
{}

void Cyclone::UI::MainUI::Initialize()
{
	mViewportManager = std::make_unique<Cyclone::UI::ViewportManager>();
	mOutliner = std::make_unique<Cyclone::UI::Outliner>();
	mToolbar = std::make_unique<Cyclone::UI::Toolbar>();
}

void Cyclone::UI::MainUI::SetDevice( ID3D11Device3 *inDevice )
{
	mViewportManager->SetDevice( inDevice );
}

void Cyclone::UI::MainUI::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	static bool showDemoMenu = false;
	if ( showDemoMenu ) ImGui::ShowDemoWindow();

	//ImGui::SetKeyOwner( ImGuiMod_Alt, 0, ImGuiInputFlags_None );

	if ( ImGui::BeginMainMenuBar() ) {
		if ( ImGui::BeginMenu( "File" ) ) {
			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "Viewports" ) ) {
			ImGui::MenuItem( "Enable VSync", nullptr, &mVerticalSyncEnabled );
			mViewportManager->MenuBarUpdate();
			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "Debug" ) ) {
			ImGui::MenuItem( "Show Demo Menu", nullptr, &showDemoMenu );

			ImGui::EndMenu();
		}

		ImGui::Separator();

		ImGui::TextDisabled( "%.0f FPS", ImGui::GetIO().Framerate );

		ImGui::EndMainMenuBar();
	}

	if ( ImGui::GetFrameCount() <= 1 ) return;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();


	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::SetNextWindowPos( viewport->WorkPos );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x, kToolbarHeight } );
	if ( ImGui::Begin( "ToolBar", nullptr, windowFlags ) ) {
		mToolbar->Update( inLevelInterface );

	}
	ImGui::End();
	ImGui::PopStyleVar( 1 );

	ImGui::SetNextWindowPos( { viewport->WorkPos.x, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { kSidebarWidth, viewport->WorkSize.y - kToolbarHeight } );
	if ( ImGui::Begin( "SideBar", nullptr, windowFlags ) ) {

	}
	ImGui::End();

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + kSidebarWidth, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x - kSidebarWidth - kOutlinerWidth, viewport->WorkSize.y - kToolbarHeight } );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
	if ( ImGui::Begin( "MainWindow", nullptr, windowFlags | ImGuiWindowFlags_NoDecoration ) ) {
		mViewportManager->Update( inDeltaTime, inLevelInterface );
	}
	ImGui::End();
	ImGui::PopStyleVar( 3 );


	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
	ImGui::SetNextWindowPos( { viewport->WorkPos.x + viewport->WorkSize.x - kOutlinerWidth, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { 256, viewport->WorkSize.y - kToolbarHeight } );
	if ( ImGui::Begin( "Outliner", nullptr, windowFlags ) ) {
		mOutliner->Update( inLevelInterface );
	}
	ImGui::End();
	ImGui::PopStyleVar( 3 );

	if ( auto &entityContext = inLevelInterface->GetEntityCtx(); entityContext.CanAquireActionLock() ) {
		if( ImGui::IsKeyChordPressed( ImGuiKey_Z | ImGuiMod_Ctrl ) ) entityContext.UndoAction( inLevelInterface->GetRegistry() );
		if( ImGui::IsKeyChordPressed( ImGuiKey_Y | ImGuiMod_Ctrl ) ) entityContext.RedoAction( inLevelInterface->GetRegistry() );

		if ( ImGui::IsKeyChordPressed( ImGuiKey_Delete ) ) {
			entityContext.BeginAction();
			for ( entt::entity entity : inLevelInterface->GetSelectionCtx().GetSelectedEntities() ) {
				entityContext.DeleteEntity( entity, inLevelInterface->GetRegistry() );
			}
			entityContext.EndAction();
		}
	}

	inLevelInterface->OnUpdateEnd();
}

void Cyclone::UI::MainUI::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	if ( ImGui::GetFrameCount() <= 1 ) return;

	mViewportManager->Render( inDeviceContext, inLevelInterface );
}
