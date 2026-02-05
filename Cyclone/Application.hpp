#pragma once

namespace Cyclone
{
	namespace UI {
		class MainUI;
	}

	namespace Core {
		class LevelInterface;
	}

	class Application
	{
	public:
		Application() noexcept;
		~Application();

		Application( Application && ) = default;
		Application &operator= ( Application && ) = default;

		Application( Application const & ) = delete;
		Application &operator= ( Application const & ) = delete;

		// Initialize device and create default resources
		void Initialize( HWND inWindow, int inWidth, int inHeight );

		// Render and update loop
		void Tick();

		// System messages
		void OnActivated();
		void OnDeactivated();
		void OnSuspending();
		void OnResuming();
		void OnWindowSizeChanged( int inWidth, int inHeight );

		// Properties
		void GetDefaultSize( int &outWidth, int &outHeight ) const noexcept;

	protected:
		void Update( float inDeltaTime );
		void Render();

		void Clear();
		void Present();

		void CreateDevice();
		void CreateResources();

		void OnDeviceLost();

		// Device resources.
		HWND                                            mWindow;
		int                                             mOutputWidth;
		int                                             mOutputHeight;

		D3D_FEATURE_LEVEL                               mFeatureLevel;
		Microsoft::WRL::ComPtr<ID3D11Device3>           mDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext3>    mDeviceContext;

		Microsoft::WRL::ComPtr<IDXGISwapChain1>         mSwapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  mRenderTargetView;

		std::unique_ptr<Cyclone::UI::MainUI>			mMainUI;
		std::unique_ptr<Cyclone::Core::LevelInterface>	mLevelInterface;
	};
}