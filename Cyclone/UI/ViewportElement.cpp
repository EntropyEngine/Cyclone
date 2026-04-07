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
	mTargetID = std::make_unique<DX::MSAAHelper>( DXGI_FORMAT_R32_UINT, DXGI_FORMAT_UNKNOWN, inAntialiasing ? 4 : 1 );
	mTargetRT = std::make_unique<DX::RenderTexture>( inBackBufferFormat );
	mClearColor = inClearColor;

	mWireframeBoxShader = std::make_unique<Cyclone::Rendering::Shader::WireframeBoxShader>();
	mEntityIndexShader = std::make_unique<Cyclone::Rendering::Shader::EntityIndexShader>();

	mWidth = 0;
	mHeight = 0;
}

Cyclone::UI::ViewportElement::~ViewportElement()
{
	mTargetMSAA->ReleaseDevice();
	mTargetID->ReleaseDevice();
	mTargetRT->ReleaseDevice();
}

void Cyclone::UI::ViewportElement::SetDevice( ID3D11Device3 *inDevice )
{
	mTargetMSAA->SetDevice( inDevice );
	mTargetID->SetDevice( inDevice );
	mTargetRT->SetDevice( inDevice );

	mWireframeBoxShader->SetDevice( inDevice );

	mEntityIndexShader->SetDevice( inDevice );
	
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
	mWireframeGridBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>( deviceContext.Get(), 16384 * 3, 16384 );
}

void Cyclone::UI::ViewportElement::UpdateViewportData( ID3D11DeviceContext *inContext )
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

	mViewportData.mDeviceContext = inContext;
	mViewportData.mEntityIndexShader = mEntityIndexShader.get();
}

ID3D11ShaderResourceView *Cyclone::UI::ViewportElement::GetOrResizeSRV( size_t inWidth, size_t inHeight )
{
	mTargetMSAA->SizeResources( inWidth, inHeight );
	mTargetID->SizeResources( inWidth, inHeight );
	mTargetRT->SizeResources( inWidth, inHeight );

	mEntityIndexShader->SizeResources( inWidth, inHeight, mTargetMSAA->GetSampleCount() );
	mViewportData.mEntitySRV = mTargetID->GetShaderResourceView();

	mWidth = inWidth;
	mHeight = inHeight;

	return mTargetRT->GetShaderResourceView();
}

void Cyclone::UI::ViewportElement::Clear( ID3D11DeviceContext3 * inDeviceContext )
{
	ID3D11RenderTargetView *renderTargetView = mTargetMSAA->GetMSAARenderTargetView();
	ID3D11RenderTargetView *renderTargetViewID = mTargetID->GetMSAARenderTargetView();
	ID3D11DepthStencilView *depthStencilView = mTargetMSAA->GetMSAADepthStencilView();

	inDeviceContext->ClearRenderTargetView( renderTargetView, mClearColor );
	inDeviceContext->ClearRenderTargetView( renderTargetViewID, DirectX::XMVECTORF32{{{ 0.0f, 0.0f, 0.0f, 0.0f }}} );
	inDeviceContext->ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

	ID3D11RenderTargetView *views[2] = { renderTargetView, renderTargetViewID };

	inDeviceContext->OMSetRenderTargets( 2, views, depthStencilView );

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
	mTargetID->SetSampleCount( inEnabled ? 4 : 1 );

	mEntityIndexShader->SizeResources( mWidth, mHeight, mTargetMSAA->GetSampleCount() );
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
	using Cyclone::Core::Component::PathCache;

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

	auto pathLambda = [&]( bool selected, bool selection, auto &view ) {
		inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );
		mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), viewMatrix, projMatrix );
		mWireframeGridEffect->Apply( inDeviceContext );

		mWireframeGridBatch->Begin();

		auto draw = [&]( const entt::entity entity ) {
			const auto &entityType = view.get<EntityType>( entity );
			const auto &position = view.get<Position>( entity ).mValue;
			const auto &rotation = view.get<Rotation>( entity ).mPitchYawRoll;
			const PathCache &pathCache = view.get<PathCache>( entity );
			const PathData &pathData = view.get<PathData>( entity );

			DirectX::XMMATRIX rotmatF = DirectX::XMMatrixRotationRollPitchYawFromVector( rotation );
			Matrix44D rotmatD = Matrix44D::sFromXMMATRIX( rotmatF );
			Vector4D rebasedEntityPosition = ( position - cameraP );

			uint32_t entityColorU32;
			if ( entity == selectedEntity ) {
				entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
			}
			else if ( selection ) {
				entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
			}
			else {
				entityColorU32 = entityManager.GetEntityTypeColor( entityType );
			}
			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

			std::vector<DirectX::VertexPositionColor> linePoints( pathCache.mArray.size() );
			//std::vector<DirectX::VertexPositionColor> linePointsL( ( pathData.mKnots.size() - 1 ) * 17 );
			//std::vector<DirectX::VertexPositionColor> linePointsR( ( pathData.mKnots.size() - 1 ) * 17 );
			for ( size_t s = 0; s < pathCache.mArray.size(); ++s ) {
				using namespace DirectX;

				DirectX::XMVECTOR P = ( rotmatD.TransformCoord3Unit( pathCache.mArray[s].mPosition ) + rebasedEntityPosition ).ToXMVECTOR();
				DirectX::XMVECTOR PN = DirectX::XMVectorScale( DirectX::XMVector3TransformCoord( pathCache.mArray[s].mNormal, rotmatF ), 0.1f );
				DirectX::XMVECTOR PB = DirectX::XMVectorScale( DirectX::XMVector3TransformCoord( pathCache.mArray[s].mTangent, rotmatF ), 0.5f );

				DirectX::XMStoreFloat3( &linePoints[s].position, P );
				DirectX::XMStoreFloat4( &linePoints[s].color, entityColorV );

				DirectX::XMVECTOR A = P - PB;
				DirectX::XMVECTOR B = P + PB;
				DirectX::XMVECTOR C = B - PN;
				DirectX::XMVECTOR D = A - PN;

				mWireframeGridBatch->DrawLine( { A, entityColorV }, { B, entityColorV } );
				mWireframeGridBatch->DrawLine( { B, entityColorV }, { C, entityColorV } );
				mWireframeGridBatch->DrawLine( { C, entityColorV }, { D, entityColorV } );
				mWireframeGridBatch->DrawLine( { D, entityColorV }, { A, entityColorV } );
			}

			mWireframeGridBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePoints.data(), linePoints.size() );
		};

		if ( !selected ) {
			for ( const entt::entity entity : view ) {
				if ( entity != selectedEntity )
				draw( entity );
			}
		}
		else if ( view.contains( selectedEntity ) ) {
			draw( selectedEntity );
		}

		mWireframeGridBatch->End();
	};

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->RSSetState( ( mTargetMSAA->GetSampleCount() > 1 ) ? mWireframeRasterStateMSAA.Get() : mWireframeRasterState.Get() );

	// Render bounding boxes (excluding paths)
	{
		inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 2 );
		{
			mWireframeBoxShader->Apply( inDeviceContext );
			mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
			mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

			auto view = cregistry.view<Position, BoundingBox, DrawTag, entt::tag<"is_selected"_hs>>( entt::exclude<PathTag> );

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 255, 0, 255 ) );
			if ( view.contains( selectedEntity ) ) {
				const auto &position = view.get<Position>( selectedEntity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( selectedEntity ).mValue;

				Vector4D rebasedEntityPosition = ( position - cameraP );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV, static_cast<uint32_t>( selectedEntity ) );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}
		{
			auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathCache, PathData, DrawTag, entt::tag<"is_selected"_hs>>();
			pathLambda( true, false, view );
		}

		inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 1 );
		{
			mWireframeBoxShader->Apply( inDeviceContext );
			mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
			mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

			auto view = cregistry.view<Position, BoundingBox, DrawTag, entt::tag<"is_selected"_hs>>( entt::exclude<PathTag> );

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 128, 0, 255 ) );
			for ( const entt::entity entity : view ) {
				if ( selectedEntity == entity ) continue;

				const auto &position = view.get<Position>( entity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

				Vector4D rebasedEntityPosition = ( position - cameraP );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV, static_cast<uint32_t>( entity ) );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}
		{
			auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathCache, PathData, DrawTag, entt::tag<"is_selected"_hs>>();
			pathLambda( false, true, view );
		}

		inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 0 );
		{

			mWireframeBoxShader->Apply( inDeviceContext );
			mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
			mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

			auto view = cregistry.view<EntityType, Position, BoundingBox, DrawTag>( entt::exclude<entt::tag<"is_selected"_hs>, PathTag> );
			for ( const entt::entity entity : view ) {
				const auto &entityType = view.get<EntityType>( entity );
				const auto &position = view.get<Position>( entity ).mValue;
				const auto &boundingBox = view.get<BoundingBox>( entity ).mValue;

				uint32_t entityColorU32 = entityManager.GetEntityTypeColor( entityType );

				DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

				Vector4D rebasedEntityPosition = ( position - cameraP );
				Vector4D rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

				mWireframeBoxShader->SetInstance( inDeviceContext, rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV, static_cast<uint32_t>( entity ) );
			}
			mWireframeBoxShader->DrawInstances( inDeviceContext );
		}
		{
			auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathCache, PathData, DrawTag>( entt::exclude<entt::tag<"is_selected"_hs>> );
			pathLambda( false, false, view );
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
