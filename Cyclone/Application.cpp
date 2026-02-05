#include "pch.h"

#include "Application.hpp"

// Cyclone includes
#include "Cyclone/UI/MainUI.hpp"
#include "Cyclone/Core/LevelInterface.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

Cyclone::Application::Application() noexcept :
	mWindow( nullptr ),
	mOutputWidth( 1920 ),
	mOutputHeight( 1080 ),
	mFeatureLevel( D3D_FEATURE_LEVEL_12_1 )
{
	// Create main UI
	mMainUI = std::make_unique<Cyclone::UI::MainUI>();

	// Create the level interface
	mLevelInterface = std::make_unique<Cyclone::Core::LevelInterface>();
}

Cyclone::Application::~Application()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	mDeviceContext->Flush();
}

void Cyclone::Application::Initialize( HWND inWindow, int inWidth, int inHeight )
{
	mWindow = inWindow;
	mOutputWidth = inWidth;
	mOutputHeight = inHeight;

	// Initialize systems
	mMainUI->Initialize();
	mLevelInterface->Initialize();

	// Create DX resources
	CreateDevice();
	CreateResources();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.Fonts->AddFontFromFileTTF( "unispace.bold.otf", 13.0f );
	
	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init( inWindow );
	ImGui_ImplDX11_Init( mDevice.Get(), mDeviceContext.Get() );
}

void Cyclone::Application::Tick()
{
	ImGuiIO& io = ImGui::GetIO();

	Update( io.DeltaTime );
	Render();
}

void Cyclone::Application::OnActivated()
{}

void Cyclone::Application::OnDeactivated()
{}

void Cyclone::Application::OnSuspending()
{}

void Cyclone::Application::OnResuming()
{}

void Cyclone::Application::OnWindowSizeChanged( int inWidth, int inHeight )
{
	if ( !mWindow )
		return;

	mOutputWidth = std::max( inWidth, 1 );
	mOutputHeight = std::max( inHeight, 1 );

	CreateResources();
}

void Cyclone::Application::GetDefaultSize( int &outWidth, int &outHeight ) const noexcept
{
	outWidth = 1920;
	outHeight = 1080;
}

void Cyclone::Application::Update( float inDeltaTime )
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	mMainUI->Update( inDeltaTime, mLevelInterface.get() );
}

void Cyclone::Application::Render()
{
	mMainUI->Render( mDeviceContext.Get(), mLevelInterface.get() );

	Clear();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );

	if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		// Update and Render additional Platform Windows
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	Present();
}

void Cyclone::Application::Clear()
{
	// Clear the views.
	mDeviceContext->ClearRenderTargetView( mRenderTargetView.Get(), DirectX::Colors::Transparent );

	mDeviceContext->OMSetRenderTargets( 1, mRenderTargetView.GetAddressOf(), nullptr );

	// Set the viewport.
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>( mOutputWidth ), static_cast<float>( mOutputHeight ), 0.f, 1.f };
	mDeviceContext->RSSetViewports( 1, &viewport );
}

void Cyclone::Application::Present()
{
	UINT syncInterval = mMainUI->IsVerticalSyncEnabled() ? 1 : 0;

	HRESULT hr = mSwapChain->Present( syncInterval, 0 );

	// If the device was reset we must completely reinitialize the renderer.
	if ( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET )
	{
		OnDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed( hr );
	}
}

void Cyclone::Application::CreateDevice()
{
	UINT creationFlags = 0;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	static const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	// Create the DX11 API device object, and get a corresponding context.
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	DX::ThrowIfFailed( D3D11CreateDevice(
		nullptr,                            // specify nullptr to use the default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		creationFlags,
		featureLevels,
		static_cast<UINT>( std::size( featureLevels ) ),
		D3D11_SDK_VERSION,
		device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
		&mFeatureLevel,                     // returns feature level of device created
		context.ReleaseAndGetAddressOf()    // returns the device immediate context
	) );

#ifndef NDEBUG
	Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
	if ( SUCCEEDED( device.As( &d3dDebug ) ) )
	{
		Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if ( SUCCEEDED( d3dDebug.As( &d3dInfoQueue ) ) )
		{
		#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_CORRUPTION, true );
			d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_ERROR, true );
		#endif
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// TODO: Add more message IDs here as needed.
			};
			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = static_cast<UINT>( std::size( hide ) );
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries( &filter );
		}
	}
#endif

	DX::ThrowIfFailed( device.As( &mDevice ) );
	DX::ThrowIfFailed( context.As( &mDeviceContext ) );

	// Set system devices
	mMainUI->SetDevice( mDevice.Get() );
	mLevelInterface->SetDevice( mDevice.Get() );
}

void Cyclone::Application::CreateResources()
{
	// Clear the previous window size specific context.
	mDeviceContext->OMSetRenderTargets( 0, nullptr, nullptr );
	mRenderTargetView.Reset();
	mDeviceContext->Flush();

	const UINT backBufferWidth = static_cast<UINT>( mOutputWidth );
	const UINT backBufferHeight = static_cast<UINT>( mOutputHeight );
	const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	constexpr UINT backBufferCount = 2;

	// If the swap chain already exists, resize it, otherwise create one.
	if ( mSwapChain )
	{
		HRESULT hr = mSwapChain->ResizeBuffers( backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0 );

		if ( hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET )
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();

			// Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
			// and correctly set up the new device.
			return;
		}
		else
		{
			DX::ThrowIfFailed( hr );
		}
	}
	else
	{
		// First, retrieve the underlying DXGI Device from the D3D Device.
		Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed( mDevice.As( &dxgiDevice ) );

		// Identify the physical adapter (GPU or card) this device is running on.
		Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed( dxgiDevice->GetAdapter( dxgiAdapter.GetAddressOf() ) );

		// And obtain the factory object that created it.
		Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
		DX::ThrowIfFailed( dxgiAdapter->GetParent( IID_PPV_ARGS( dxgiFactory.GetAddressOf() ) ) );

		// Create a descriptor for the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = backBufferWidth;
		swapChainDesc.Height = backBufferHeight;
		swapChainDesc.Format = backBufferFormat;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = backBufferCount;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		// Create a SwapChain from a Win32 window.
		DX::ThrowIfFailed( dxgiFactory->CreateSwapChainForHwnd(
			mDevice.Get(),
			mWindow,
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			mSwapChain.ReleaseAndGetAddressOf()
		) );

		// This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
		DX::ThrowIfFailed( dxgiFactory->MakeWindowAssociation( mWindow, DXGI_MWA_NO_ALT_ENTER ) );
	}

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed( mSwapChain->GetBuffer( 0, IID_PPV_ARGS( backBuffer.GetAddressOf() ) ) );

	// Create a view interface on the rendertarget to use on bind.
	DX::ThrowIfFailed( mDevice->CreateRenderTargetView( backBuffer.Get(), nullptr, mRenderTargetView.ReleaseAndGetAddressOf() ) );
}

void Cyclone::Application::OnDeviceLost()
{
	mRenderTargetView.Reset();
	mSwapChain.Reset();
	mDeviceContext.Reset();
	mDevice.Reset();

	mLevelInterface->ReleaseResources();

	CreateDevice();

	CreateResources();
}
