#include "pch.h"
#include "Cyclone/UI/ViewportElement.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/Visible.hpp"
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone UI
#include "Cyclone/UI/Tool/BaseTool.hpp"

// Cyclone math
#include "Cyclone/Math/Matrix.hpp"

// DX Includes
#include <DirectXHelpers.h>

Cyclone::UI::ViewportElement::ViewportElement( DXGI_FORMAT inBackBufferFormat, DXGI_FORMAT inDepthBufferFormat, const DirectX::XMVECTORF32 inClearColor, bool inAntialiasing )
{
	mTargetMSAA = std::make_unique<DX::MSAAHelper>( inBackBufferFormat, inDepthBufferFormat, inAntialiasing ? 4 : 1 );
	mTargetRT = std::make_unique<DX::RenderTexture>( inBackBufferFormat );
	mClearColor = inClearColor;

	mWireframeBoxShader = std::make_unique<Cyclone::Rendering::Shader::WireframeBoxShader>();

	mWidth = 0;
	mHeight = 0;
}

Cyclone::UI::ViewportElement::~ViewportElement()
{
	mTargetMSAA->ReleaseDevice();
	mTargetRT->ReleaseDevice();
}

void Cyclone::UI::ViewportElement::SetDevice( ID3D11Device3 *inDevice )
{
	mTargetMSAA->SetDevice( inDevice );
	mTargetRT->SetDevice( inDevice );

	mWireframeBoxShader->SetDevice( inDevice );
	
	// Rasterizer States
	{
		CD3D11_RASTERIZER_DESC rssDesc( D3D11_DEFAULT );
		rssDesc.FillMode = D3D11_FILL_WIREFRAME;
		rssDesc.CullMode = D3D11_CULL_NONE;
		rssDesc.FrontCounterClockwise = TRUE;
		DX::ThrowIfFailed( inDevice->CreateRasterizerState( &rssDesc, mWireframeRasterState.ReleaseAndGetAddressOf() ) );

		rssDesc.MultisampleEnable = TRUE;
		DX::ThrowIfFailed( inDevice->CreateRasterizerState( &rssDesc, mWireframeRasterStateMSAA.ReleaseAndGetAddressOf() ) );
	}

	// Depth Stencil States
	{
		D3D11_DEPTH_STENCIL_DESC dssDesc = {};

		dssDesc.DepthEnable = TRUE;
		dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dssDesc.DepthFunc = D3D11_COMPARISON_LESS;

		dssDesc.StencilEnable = TRUE;
		dssDesc.StencilReadMask = 0xFF;
		dssDesc.StencilWriteMask = 0xFF;

		dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;

		dssDesc.BackFace = dssDesc.FrontFace;

		DX::ThrowIfFailed( inDevice->CreateDepthStencilState( &dssDesc, mLayeredDepthState.ReleaseAndGetAddressOf() ) );
	}

	mCommonStates = std::make_unique<DirectX::CommonStates>( inDevice );

	mWireframeGridEffect = std::make_unique<DirectX::BasicEffect>( inDevice );
	mWireframeGridEffect->SetVertexColorEnabled( true );
	DX::ThrowIfFailed( DirectX::CreateInputLayoutFromEffect<DirectX::VertexPositionColor>( inDevice, mWireframeGridEffect.get(), mWireframeGridInputLayout.ReleaseAndGetAddressOf() ) );

	Microsoft::WRL::ComPtr<ID3D11DeviceContext3> deviceContext;
	inDevice->GetImmediateContext3( deviceContext.GetAddressOf() );
	mWireframeGridBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>( deviceContext.Get() );
}

void Cyclone::UI::ViewportElement::UpdateViewportData()
{
	mViewportData.mViewSize = ImGui::GetWindowSize();
	mViewportData.mViewOrigin = ImGui::GetCursorScreenPos();
	mViewportData.mDrawList = ImGui::GetWindowDrawList();
	mViewportData.mIsActive = false;

	if ( mTargetMSAA->GetSampleCount() <= 1 ) {
		mViewportData.mDrawList->Flags &= ~( ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedLinesUseTex | ImDrawListFlags_AntiAliasedFill );
	}

	// Split channels into 4 planes
	mViewportData.mDrawList->ChannelsSplit( 4 );
	mViewportData.mDrawList->ChannelsSetCurrent( 0 );
}

ID3D11ShaderResourceView *Cyclone::UI::ViewportElement::GetOrResizeSRV( size_t inWidth, size_t inHeight )
{
	mTargetMSAA->SizeResources( inWidth, inHeight );
	mTargetRT->SizeResources( inWidth, inHeight );

	mWidth = inWidth;
	mHeight = inHeight;

	return mTargetRT->GetShaderResourceView();
}

void Cyclone::UI::ViewportElement::Clear( ID3D11DeviceContext3 * inDeviceContext )
{
	ID3D11RenderTargetView *renderTargetView = mTargetMSAA->GetMSAARenderTargetView();
	ID3D11DepthStencilView *depthStencilView = mTargetMSAA->GetMSAADepthStencilView();

	inDeviceContext->ClearRenderTargetView( renderTargetView, mClearColor );
	inDeviceContext->ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

	inDeviceContext->OMSetRenderTargets( 1, &renderTargetView, depthStencilView );

	D3D11_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>( mWidth ), static_cast<float>( mHeight ), 0.0f, 1.0f };
	inDeviceContext->RSSetViewports( 1, &viewport );
}

void Cyclone::UI::ViewportElement::Resolve( ID3D11DeviceContext3 * inDeviceContext )
{
	mTargetMSAA->Resolve( inDeviceContext, mTargetRT->GetRenderTarget() );
}

void Cyclone::UI::ViewportElement::ToggleAntialiasing( bool inEnabled )
{
	mTargetMSAA->SetSampleCount( inEnabled ? 4 : 1 );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElement::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const std::span<std::unique_ptr<Tool::BaseTool>> inTools )
{
	using Cyclone::Math::Vector4D;
	using Cyclone::Math::Matrix44D;

	using Cyclone::Core::Component::EntityType;
	using Cyclone::Core::Component::Position;
	using Cyclone::Core::Component::Rotation;
	using Cyclone::Core::Component::BoundingBox;
	using Cyclone::Core::Component::PathTag;
	using Cyclone::Core::Component::PathData;

	using DrawTag = ViewportTypeTraits<T>::DrawTag;

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityManager = inLevelInterface->GetEntityManager();

	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();

	const entt::registry &cregistry = inLevelInterface->GetRegistry();

	const auto &gridContext = inLevelInterface->GetGridCtx();
	Vector4D cameraP = T == EViewportType::Perspective ? inLevelInterface->GetPerspectiveCtx().mCenter3D : inLevelInterface->GetOrthographicCtx().mCenter2D;

	const DirectX::XMMATRIX viewMatrix = mViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = mViewportData.mProjMatrix;

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mWireframeRasterStateMSAA.Get() : mWireframeRasterState.Get() );

	// Render bounding boxes (excluding paths)
	{
		mWireframeBoxShader->Apply( inDeviceContext );
		mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
		mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

		{
			auto view = cregistry.view<Position, BoundingBox, DrawTag, entt::tag<"is_selected"_hs>>( entt::exclude<PathTag> );

			inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 2 );
			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 255, 0, 255 ) );
			if ( view.contains( selectedEntity ) ) {
				const auto &position = view.get<Position>( selectedEntity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( selectedEntity ).mValue;

				Vector4D rebasedEntityPosition = ( position - cameraP );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );

			inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 1 );
			entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 128, 0, 255 ) );
			for ( const entt::entity entity : view ) {
				if ( selectedEntity == entity ) continue;

				const auto &position = view.get<Position>( entity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

				Vector4D rebasedEntityPosition = ( position - cameraP );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}

		{
			inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 0 );
			auto view = cregistry.view<EntityType, Position, BoundingBox, DrawTag>( entt::exclude<entt::tag<"is_selected"_hs>, PathTag> );
			//view.use<BoundingBox>();
			for ( const entt::entity entity : view ) {
				const auto &entityType = view.get<EntityType>( entity );
				const auto &position = view.get<Position>( entity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

				uint32_t entityColorU32 = entityManager.GetEntityTypeColor( entityType );

				DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

				Vector4D rebasedEntityPosition = ( position - cameraP );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}
	}

	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );
	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), viewMatrix, projMatrix );
	mWireframeGridEffect->Apply( inDeviceContext );

	// Switch to depth buffer
	inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 0 );

	// Render paths
	{
		{
			mWireframeGridBatch->Begin();

			auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathData, DrawTag>();
			//view.use<BoundingBox>();
			for ( const entt::entity entity : view ) {
				const auto &entityType = view.get<EntityType>( entity );
				const auto &position = view.get<Position>( entity ).mValue;
				const auto &rotation = view.get<Rotation>( entity ).mPitchYawRoll;
				const PathData &pathData = view.get<PathData>( entity );

				Matrix44D rotmat = Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( rotation ) );
				Vector4D rebasedEntityPosition = ( position - cameraP );

				uint32_t entityColorU32;
				if ( entity == selectedEntity ) {
					entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
				}
				else if ( selectedEntities.contains( entity ) ) {
					entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
				}
				else {
					entityColorU32 = entityManager.GetEntityTypeColor( entityType );
				}
				DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

				std::vector<DirectX::VertexPositionColor> linePoints( ( pathData.mKnots.size() - 1 ) * 65 );
				//std::vector<DirectX::VertexPositionColor> linePointsL( ( pathData.mKnots.size() - 1 ) * 17 );
				//std::vector<DirectX::VertexPositionColor> linePointsR( ( pathData.mKnots.size() - 1 ) * 17 );
				for ( size_t s = 0; s + 1 < pathData.mKnots.size(); ++s ) {
					for ( size_t i = 0; i <= 64; ++i ) {
						float u = static_cast<float>( i ) / 64;
						DirectX::XMStoreFloat3( &linePoints[s * 65 + i].position, ( rotmat.TransformCoord3Unit( pathData.Interpolate( s, u ) ) + rebasedEntityPosition ).ToXMVECTOR() );
						DirectX::XMStoreFloat4( &linePoints[s * 65 + i].color, entityColorV );

						if ( i % 4 == 0 ) {
							using namespace DirectX;

							//DirectX::XMStoreFloat3( &linePointsL[s * 17 + i / 4].position, ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, 0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR() );
							//DirectX::XMStoreFloat4( &linePointsL[s * 17 + i / 4].color, entityColorV * 0.75f );

							//DirectX::XMStoreFloat3( &linePointsR[s * 17 + i / 4].position, ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, -0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR() );
							//DirectX::XMStoreFloat4( &linePointsR[s * 17 + i / 4].color, entityColorV * 0.75f );


							DirectX::XMVECTOR A = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, -0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR();
							DirectX::XMVECTOR B = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, 0.5, 0 ) ) + rebasedEntityPosition ).ToXMVECTOR();
							DirectX::XMVECTOR C = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, 0.5, -0.1 ) ) + rebasedEntityPosition ).ToXMVECTOR();
							DirectX::XMVECTOR D = ( rotmat.TransformCoord3Unit( pathData.InterpolateUVW( s, u, -0.5, -0.1 ) ) + rebasedEntityPosition ).ToXMVECTOR();

							mWireframeGridBatch->DrawLine( { A, entityColorV }, { B, entityColorV } );
							mWireframeGridBatch->DrawLine( { B, entityColorV }, { C, entityColorV } );
							mWireframeGridBatch->DrawLine( { C, entityColorV }, { D, entityColorV } );
							mWireframeGridBatch->DrawLine( { D, entityColorV }, { A, entityColorV } );


							//mWireframeGridBatch->DrawLine( linePointsL[s * 17 + i / 4], linePointsR[s * 17 + i / 4] );
						}

					}
				}

				mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePoints.data(), linePoints.size() );
				//mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsL.data(), linePointsL.size() );
				//mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsR.data(), linePointsR.size() );

				// TODO
				// Draw the path
				// TODO
			}

			mWireframeGridBatch->End();
		}
	}

	// Call all tool renders with depth enabled
	inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 0 );
	{
		mWireframeGridBatch->Begin();
		for ( auto &tool : inTools ) {
			tool->OnRender( T, inLevelInterface, mViewportData, mWireframeGridBatch.get() );
		}
		mWireframeGridBatch->End();

	}
}

template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::Perspective>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const std::span<std::unique_ptr<Tool::BaseTool>> );
template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::TopXZ>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const std::span<std::unique_ptr<Tool::BaseTool>> );
template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::FrontXY>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const std::span<std::unique_ptr<Tool::BaseTool>> );
template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::SideYZ>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const std::span<std::unique_ptr<Tool::BaseTool>> );
