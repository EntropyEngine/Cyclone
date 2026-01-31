#include "pch.h"

#include "Application.hpp"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

Application::Application() noexcept :
	mWindow( nullptr ),
	mOutputWidth( 1920 ),
	mOutputHeight( 1080 ),
	mFeatureLevel( D3D_FEATURE_LEVEL_12_1 )
{}

Application::~Application()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Application::Initialize( HWND inWindow, int inWidth, int inHeight )
{
	mWindow = inWindow;
	mOutputWidth = inWidth;
	mOutputHeight = inHeight;

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
	
	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init( inWindow );
	ImGui_ImplDX11_Init( mDevice.Get(), mDeviceContext.Get());
}

void Application::Tick()
{
	Update( 1.0f / 60 );
	Render();
}

void Application::OnActivated()
{}

void Application::OnDeactivated()
{}

void Application::OnSuspending()
{}

void Application::OnResuming()
{}

void Application::OnWindowSizeChanged( int inWidth, int inHeight )
{
	if ( !mWindow )
		return;

	mOutputWidth = std::max( inWidth, 1 );
	mOutputHeight = std::max( inHeight, 1 );

	CreateResources();
}

void Application::GetDefaultSize( int &outWidth, int &outHeight ) const noexcept
{
	outWidth = 1920;
	outHeight = 1080;
}

void Application::Update( float inDeltaTime )
{}

void Application::Render()
{
	Clear();

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow(); // Show demo window! :)

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

void Application::Clear()
{
	// Clear the views.
	mDeviceContext->ClearRenderTargetView( mRenderTargetView.Get(), DirectX::Colors::CornflowerBlue );
	mDeviceContext->ClearDepthStencilView( mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

	mDeviceContext->OMSetRenderTargets( 1, mRenderTargetView.GetAddressOf(), mDepthStencilView.Get() );

	// Set the viewport.
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>( mOutputWidth ), static_cast<float>( mOutputHeight ), 0.f, 1.f };
	mDeviceContext->RSSetViewports( 1, &viewport );
}

void Application::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = mSwapChain->Present( 0, 0 );

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

void Application::CreateDevice()
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

	// TODO: Initialize device dependent objects here (independent of window size).
}

void Application::CreateResources()
{
	// Clear the previous window size specific context.
	mDeviceContext->OMSetRenderTargets( 0, nullptr, nullptr );
	mRenderTargetView.Reset();
	mDepthStencilView.Reset();
	mDeviceContext->Flush();

	const UINT backBufferWidth = static_cast<UINT>( mOutputWidth );
	const UINT backBufferHeight = static_cast<UINT>( mOutputHeight );
	const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
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

	// Allocate a 2-D surface as the depth/stencil buffer and
	// create a DepthStencil view on this surface to use on bind.
	CD3D11_TEXTURE2D_DESC depthStencilDesc( depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL );

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed( mDevice->CreateTexture2D( &depthStencilDesc, nullptr, depthStencil.GetAddressOf() ) );

	DX::ThrowIfFailed( mDevice->CreateDepthStencilView( depthStencil.Get(), nullptr, mDepthStencilView.ReleaseAndGetAddressOf() ) );

	// TODO: Initialize windows-size dependent objects here.
}

void Application::OnDeviceLost()
{
	mDepthStencilView.Reset();
	mRenderTargetView.Reset();
	mSwapChain.Reset();
	mDeviceContext.Reset();
	mDevice.Reset();

	CreateDevice();

	CreateResources();
}
