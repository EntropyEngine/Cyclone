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
#include "Cyclone/UI/Sidebar.hpp"
#include "Cyclone/UI/ObjectProperties.hpp"

// Cyclone Utils
#include "Cyclone/Util/String.hpp"

// ImGui includes
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
	mSidebar = std::make_unique<Cyclone::UI::Sidebar>();
	mSidebar->Init();
}

void Cyclone::UI::MainUI::SetDevice( ID3D11Device3 *inDevice )
{
	mViewportManager->SetDevice( inDevice );
}

void Cyclone::UI::MainUI::Update( ID3D11DeviceContext3 *inDeviceContext, float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	static bool showDemoMenu = false;
	static bool showMetricsMenu = false;
	if ( showDemoMenu ) ImGui::ShowDemoWindow();
	if ( showMetricsMenu ) ImGui::ShowMetricsWindow();

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
			ImGui::MenuItem( "Show Metrics Menu", nullptr, &showMetricsMenu );

			ImGui::EndMenu();
		}

		ImGui::Separator();

		ImGui::TextDisabled( "%.0f FPS", ImGui::GetIO().Framerate );

		ImGui::EndMainMenuBar();
	}

	if ( ImGui::GetFrameCount() <= 1 ) return;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;

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
		mSidebar->Update( inLevelInterface );
	}
	ImGui::End();

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + kSidebarWidth, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x - kSidebarWidth - kOutlinerWidth, viewport->WorkSize.y - kToolbarHeight } );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
	if ( ImGui::Begin( "MainWindow", nullptr, windowFlags | ImGuiWindowFlags_NoDecoration ) ) {
		mViewportManager->Update( inDeviceContext, inDeltaTime, inLevelInterface, mSidebar->GetTools() );
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

	auto &registry = inLevelInterface->GetRegistry();
	auto &entityManager = inLevelInterface->GetEntityManager();

	// Show object properties
	for ( entt::entity entity : std::set( entityManager.GetOpenedProperties() ) )
	{
		bool isOpen = registry.all_of<Core::Component::EntityType>( entity );
		ImGui::SetNextWindowSizeConstraints( { 480, 480 }, { FLT_MAX, FLT_MAX } );
		if ( isOpen ) {
			if ( ImGui::Begin( Cyclone::Util::PrefixString( "Entity: ", entity ), &isOpen, ImGuiWindowFlags_AlwaysVerticalScrollbar ) ) {
				ObjectProperties().ShowWindow( inLevelInterface, entity );
			}
			ImGui::End();
		}
		if ( !isOpen ) entityManager.CloseEntityProperties( entity );
	}

	if ( entityManager.CanAquireActionLock() ) {
		auto &selectionContext = inLevelInterface->GetSelectionCtx();

		if ( ImGui::GetKeyOwner( ImGuiKey_F24 ) == ImGuiKeyOwner_NoOwner ) {
			if ( ImGui::IsKeyChordPressed( ImGuiKey_Z | ImGuiMod_Ctrl, ImGuiInputFlags_None, ImGuiKeyOwner_NoOwner ) ) entityManager.UndoAction( registry );
			if ( ImGui::IsKeyChordPressed( ImGuiKey_Y | ImGuiMod_Ctrl, ImGuiInputFlags_None, ImGuiKeyOwner_NoOwner ) ) entityManager.RedoAction( registry );

			if ( ImGui::IsKeyChordPressed( ImGuiKey_A | ImGuiMod_Ctrl, ImGuiInputFlags_None, ImGuiKeyOwner_NoOwner ) ) mViewportManager->ResizeViewports();

			if ( mSidebar->GetTools().mCurrentCategory == Tool::ECategory::Object ) {
				if ( ImGui::IsKeyChordPressed( ImGuiKey_Delete, ImGuiInputFlags_None, ImGuiKeyOwner_NoOwner ) && !selectionContext.GetSelectedEntities().empty() ) {
					entityManager.BeginAction();
					for ( entt::entity entity : selectionContext.GetSelectedEntities() ) {
						entityManager.DeleteEntity( entity, registry );
					}
					entityManager.EndAction( registry );
				}

				if ( ImGui::IsKeyChordPressed( ImGuiKey_H | ImGuiMod_Ctrl, ImGuiInputFlags_None, ImGuiKeyOwner_NoOwner ) && !selectionContext.GetSelectedEntities().empty() ) {
					entityManager.BeginAction();
					for ( entt::entity entity : selectionContext.GetSelectedEntities() ) {
						registry.get<Cyclone::Core::Component::Visible>( entity ) = static_cast<Cyclone::Core::Component::Visible>( false );
						entityManager.UpdateEntity( entity, registry );
					}
					entityManager.EndAction( registry );
				}
			}

			for ( const auto &tool : mSidebar->GetTools().mTools ) {
				tool->OnShortcut( inLevelInterface );
			}
		}
	}

	inLevelInterface->OnUpdateEnd();
}

void Cyclone::UI::MainUI::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface )
{
	if ( ImGui::GetFrameCount() <= 1 ) return;

	mViewportManager->Render( inDeviceContext, inLevelInterface, mSidebar->GetTools() );
}
