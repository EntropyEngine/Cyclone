#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone UI
#include "Cyclone/UI/ViewportData.hpp"
#include "Cyclone/UI/ViewportType.hpp"

// Common includes
#include <MSAAHelper.h>
#include <RenderTexture.h>

// DX Includes
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

// Rendering includes
#include "Cyclone/Rendering/Shader/WireframeBoxShader.hpp"
#include "Cyclone/Rendering/Shader/WireframePrimitiveShader.hpp"
#include "Cyclone/Rendering/Shader/EntityIndexShader.hpp"

namespace Cyclone::Core {
	class LevelInterface;
}

namespace Cyclone::UI
{
	namespace Tool {
		class BaseTool;
		struct ToolChanger;
	}

	class ViewportElement : public Cyclone::Util::NonCopyable
	{
	public:
		ViewportElement( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor, bool inAntialiasing );
		virtual ~ViewportElement();

		void SetDevice( ID3D11Device3 *inDevice );
		void UpdateViewportData( ID3D11DeviceContext *inContext );
		const ViewportData &GetViewportData() const { return mViewportData; }

		ID3D11ShaderResourceView *GetOrResizeSRV( size_t inWidth, size_t inHeight );
		void Clear( ID3D11DeviceContext3 *inDeviceContext );
		void Resolve( ID3D11DeviceContext3 *inDeviceContext );

		size_t GetWidth() const  { return mWidth; }
		size_t GetHeight() const { return mHeight; }

		void ToggleAntialiasing( bool inEnabled );

		using VertexPositionColorID = Cyclone::Rendering::Shader::WireframePrimitiveShader::VertexPositionColorID;

	protected:
		template<Cyclone::UI::EViewportType T>
		void Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const Tool::ToolChanger &inTools );

		std::unique_ptr<DX::MSAAHelper>				mTargetMSAA;
		std::unique_ptr<DX::MSAAHelper>				mTargetID;
		std::unique_ptr<DX::RenderTexture>			mTargetRT;

		std::unique_ptr<Cyclone::Rendering::Shader::WireframeBoxShader> mWireframeBoxShader;
		std::unique_ptr<Cyclone::Rendering::Shader::WireframePrimitiveShader> mWireframePrimitiveShader;
		std::unique_ptr<Cyclone::Rendering::Shader::EntityIndexShader> mEntityIndexShader;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> mWireframeRasterState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> mWireframeRasterStateMSAA;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mLayeredDepthState;

		std::unique_ptr<DirectX::PrimitiveBatch<VertexPositionColorID>> mWireframePrimitiveBatch;
		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> mWireframeGridBatch;
		std::unique_ptr<DirectX::BasicEffect>		mWireframeGridEffect;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	mWireframeGridInputLayout;
		std::unique_ptr<DirectX::CommonStates>		mCommonStates;

		ViewportData								mViewportData;

		size_t										mWidth;
		size_t										mHeight;

		DirectX::XMVECTORF32						mClearColor;
	};
}