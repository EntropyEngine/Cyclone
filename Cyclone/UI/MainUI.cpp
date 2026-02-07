#include "pch.h"
#include "Cyclone/UI/MainUI.hpp"

// Cyclone UI includes
#include "Cyclone/UI/ViewportManager.hpp"
#include "Cyclone/UI/Outliner.hpp"

// Cyclone Core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
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
}

void Cyclone::UI::MainUI::SetDevice( ID3D11Device3 *inDevice )
{
	mViewportManager->SetDevice( inDevice );
}

void Cyclone::UI::MainUI::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inEntityInterface )
{
	static bool showDemoMenu = false;
	if ( showDemoMenu ) ImGui::ShowDemoWindow();

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

		ImGui::EndMainMenuBar();
	}

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos( viewport->WorkPos );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x, kToolbarHeight } );
	if ( ImGui::Begin( "ToolBar", nullptr, windowFlags ) ) {
		mViewportManager->ToolbarUpdate();
	}
	ImGui::End();

	ImGui::SetNextWindowPos( { viewport->WorkPos.x, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { kSidebarWidth, viewport->WorkSize.y - kToolbarHeight } );
	if ( ImGui::Begin( "SideBar", nullptr, windowFlags ) ) {

	}
	ImGui::End();

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + kSidebarWidth, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { viewport->WorkSize.x - kSidebarWidth - kOutlinerWidth, viewport->WorkSize.y - kToolbarHeight } );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f } );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f } );
	if ( ImGui::Begin( "MainWindow", nullptr, windowFlags | ImGuiWindowFlags_NoDecoration ) ) {
		mViewportManager->Update( inDeltaTime );
	}
	ImGui::End();
	ImGui::PopStyleVar( 2 );

	ImGui::SetNextWindowPos( { viewport->WorkPos.x + viewport->WorkSize.x - kOutlinerWidth, viewport->WorkPos.y + kToolbarHeight } );
	ImGui::SetNextWindowSize( { 256, viewport->WorkSize.y - kToolbarHeight } );
	if ( ImGui::Begin( "Outliner", nullptr, windowFlags ) ) {

		if ( ImGui::BeginTable( "Entity List", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp ) ) {

			ImGui::TableSetupColumn( "ID" );
			ImGui::TableSetupColumn( "Type" );
			ImGui::TableSetupColumn( "Position" );
			ImGui::TableHeadersRow();

			const entt::registry &cregistry = inEntityInterface->GetRegistry();
			cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position>().each( [&inEntityInterface]( const entt::entity inEntity, const Cyclone::Core::Component::EntityType &inEntityType, const Cyclone::Core::Component::Position &inPosition ) {
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex( 0 );
				ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns;
				if ( inEntityInterface->GetSelectedEntity() == inEntity ) selectionFlags |= ImGuiSelectableFlags_Highlight;

				if ( ImGui::Selectable( std::format( "{}", static_cast<size_t>( inEntity ) ).c_str(), inEntityInterface->GetSelectedEntities().contains( inEntity ), selectionFlags ) ) {
					if ( ImGui::GetIO().KeyCtrl ) {
						if ( inEntityInterface->GetSelectedEntity() == inEntity ) {
							inEntityInterface->DeselectEntity( inEntity );
						}
						else {
							inEntityInterface->AddSelectedEntity( inEntity );
						}
					}
					else {
						inEntityInterface->SetSelectedEntity( inEntity );
					}
				};

				ImGui::TableSetColumnIndex( 1 );
				ImGui::Text( Cyclone::Core::Entity::EntityTypeRegistry::GetEntityTypeName( inEntityType ) );

				ImGui::TableSetColumnIndex( 2 );
				ImGui::Text( "% 7.2f\n% 7.2f\n% 7.2f", inPosition.GetX(), inPosition.GetY(), inPosition.GetZ() );
			} );

			ImGui::EndTable();
		}

	}
	ImGui::End();

	//ImGui::PopStyleVar();

}

void Cyclone::UI::MainUI::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inEntityInterface )
{
	mViewportManager->RenderPerspective( inDeviceContext );
	mViewportManager->RenderTop( inDeviceContext );
	mViewportManager->RenderFront( inDeviceContext );
	mViewportManager->RenderSide( inDeviceContext );
}
