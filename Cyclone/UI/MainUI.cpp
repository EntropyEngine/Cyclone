#include "pch.h"

#include "Cyclone/UI/MainUI.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

Cyclone::UI::MainUI::MainUI() noexcept :
	mVerticalSyncEnabled( false )
{}

void Cyclone::UI::MainUI::Initialize()
{
	// Create fullscreen dockspace
	ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
}

void Cyclone::UI::MainUI::Update( float inDeltaTime )
{

}