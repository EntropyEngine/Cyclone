// Cyclone.cpp : Defines the entry point for the application.
//

#include "pch.h"

#include "main.h"
#include "Cyclone/Application.hpp"

#include <imgui.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#define MAX_LOADSTRING 100

// Global Variables
namespace
{
	std::unique_ptr<Cyclone::Application> gApplication;
}

// Global Windows Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass( HINSTANCE hInstance );
BOOL                InitInstance( HINSTANCE, int );
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR CALLBACK    About( HWND, UINT, WPARAM, LPARAM );

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int APIENTRY wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );
	UNREFERENCED_PARAMETER( nCmdShow );

	// Verify CPU support
	if ( !DirectX::XMVerifyCPUSupport() ) return 1;

	// Intialise multithreaded support
	if ( FAILED( CoInitializeEx( nullptr, COINITBASE_MULTITHREADED ) ) ) return 1;

	// Create application instance
	gApplication = std::make_unique<Cyclone::Application>();

	// Initialize global strings
	LoadStringW( hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
	LoadStringW( hInstance, IDC_CYCLONE, szWindowClass, MAX_LOADSTRING );
	
	// Register class
	if ( !MyRegisterClass( hInstance ) ) return 1;

	// Create window and initialize application
	if ( !InitInstance( hInstance, SW_MAXIMIZE ) ) return 1;

	HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_CYCLONE ) );

	MSG msg{};

	// Main message loop:
	while ( WM_QUIT != msg.message )
	{
		if ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		else {
			gApplication->Tick();
		}

	}

	gApplication.reset();

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<IDXGIDebug> debugDev;
	HRESULT hr = DXGIGetDebugInterface1( 0, IID_PPV_ARGS( debugDev.GetAddressOf() ) );
	hr = debugDev->ReportLiveObjects( DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL );
	debugDev.Reset();
#endif

	return (int) msg.wParam;
}

ATOM MyRegisterClass( HINSTANCE hInstance )
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof( WNDCLASSEX );

	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_CYCLONE ) );
	wcex.hCursor        = LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground  = (HBRUSH) ( COLOR_WINDOW + 1 );
	wcex.lpszMenuName   = MAKEINTRESOURCEW( IDC_CYCLONE );
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm        = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

	return RegisterClassExW( &wcex );
}

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
	hInst = hInstance; // Store instance handle in our global variable

	// Create window
	int w, h;
	gApplication->GetDefaultSize( w, h );

	RECT rc = { 0, 0, static_cast<LONG>( w ), static_cast<LONG>( h ) };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

	HWND hWnd = CreateWindowW( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, gApplication.get() );
	if ( !hWnd ) {
		return FALSE;
	}

	ShowWindow( hWnd, nCmdShow );

	GetClientRect( hWnd, &rc );

	gApplication->Initialize( hWnd, rc.right - rc.left, rc.bottom - rc.top );

	return TRUE;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;
	// TODO: Set s_fullscreen to true if defaulting to fullscreen.

	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	auto application = reinterpret_cast<Cyclone::Application *>( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );

	switch ( message )
	{
		case WM_CREATE:
			if ( lParam )
			{
				auto params = reinterpret_cast<LPCREATESTRUCTW>( lParam );
				SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( params->lpCreateParams ) );
			}
			break;

		case WM_PAINT:
			if ( s_in_sizemove && application )
			{
				application->Tick();
			}
			else
			{
				PAINTSTRUCT ps;
				std::ignore = BeginPaint( hWnd, &ps );
				EndPaint( hWnd, &ps );
			}
			break;

		case WM_SIZE:
			if ( wParam == SIZE_MINIMIZED )
			{
				if ( !s_minimized )
				{
					s_minimized = true;
					if ( !s_in_suspend && application )
						application->OnSuspending();
					s_in_suspend = true;
				}
			}
			else if ( s_minimized )
			{
				s_minimized = false;
				if ( s_in_suspend && application )
					application->OnResuming();
				s_in_suspend = false;
			}
			else if ( !s_in_sizemove && application )
			{
				application->OnWindowSizeChanged( LOWORD( lParam ), HIWORD( lParam ) );
			}
			break;

		case WM_ENTERSIZEMOVE:
			s_in_sizemove = true;
			break;

		case WM_EXITSIZEMOVE:
			s_in_sizemove = false;
			if ( application )
			{
				RECT rc;
				GetClientRect( hWnd, &rc );

				application->OnWindowSizeChanged( rc.right - rc.left, rc.bottom - rc.top );
			}
			break;

		case WM_GETMINMAXINFO:
			if ( lParam )
			{
				auto info = reinterpret_cast<MINMAXINFO*>( lParam );
				info->ptMinTrackSize.x = 320;
				info->ptMinTrackSize.y = 200;
			}
			break;

		case WM_ACTIVATEAPP:
			if ( application )
			{
				if ( wParam )
				{
					application->OnActivated();
				}
				else
				{
					application->OnDeactivated();
				}
			}
			break;

		case WM_POWERBROADCAST:
			switch ( wParam )
			{
				case PBT_APMQUERYSUSPEND:
					if ( !s_in_suspend && application )
						application->OnSuspending();
					s_in_suspend = true;
					return TRUE;

				case PBT_APMRESUMESUSPEND:
					if ( !s_minimized )
					{
						if ( s_in_suspend && application )
							application->OnResuming();
						s_in_suspend = false;
					}
					return TRUE;

				default:
					break;
			}
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		case WM_SYSKEYDOWN:
			if ( wParam == VK_RETURN && ( lParam & 0x60000000 ) == 0x20000000 )
			{
				// Implements the classic ALT+ENTER fullscreen toggle
				if ( s_fullscreen )
				{
					SetWindowLongPtr( hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW );
					SetWindowLongPtr( hWnd, GWL_EXSTYLE, 0 );

					int width = 800;
					int height = 600;
					if ( application )
						application->GetDefaultSize( width, height );

					ShowWindow( hWnd, SW_SHOWNORMAL );

					SetWindowPos( hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED );
				}
				else
				{
					SetWindowLongPtr( hWnd, GWL_STYLE, WS_POPUP );
					SetWindowLongPtr( hWnd, GWL_EXSTYLE, WS_EX_TOPMOST );

					SetWindowPos( hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

					ShowWindow( hWnd, SW_SHOWMAXIMIZED );
				}

				s_fullscreen = !s_fullscreen;
			}
			break;

		case WM_MENUCHAR:
			// A menu is active and the user presses a key that does not correspond
			// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
			return MAKELRESULT( 0, MNC_CLOSE );

		default:
			break;
	}

	return DefWindowProc( hWnd, message, wParam, lParam );
}
