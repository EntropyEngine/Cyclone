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
	mWireframePrimitiveShader = std::make_unique<Cyclone::Rendering::Shader::WireframePrimitiveShader>();
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
	mWireframePrimitiveShader->SetDevice( inDevice );

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
	mWireframePrimitiveBatch = std::make_unique<DirectX::PrimitiveBatch<VertexPositionColorID>>( deviceContext.Get(), 16384 * 3, 16384 );
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
void Cyclone::UI::ViewportElement::Render( ID3D11DeviceContext3 *inDeviceContext, Cyclone::Core::LevelInterface *inLevelInterface, const Tool::ToolChanger &inTools )
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
	using Cyclone::Core::Component::PathSelection;

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

	const uint32_t colSelected = inTools.mCurrentCategory == Tool::ECategory::Object ? Cyclone::Util::ColorU32( 255, 255, 0, 255 ) : Cyclone::Util::ColorU32( 255, 128, 0, 255 );
	const uint32_t colSelection = inTools.mCurrentCategory == Tool::ECategory::Object ? Cyclone::Util::ColorU32( 255, 128, 0, 255 ) : Cyclone::Util::ColorU32( 255, 0, 0, 255 );

	const bool editPathMode = inTools.mCurrentCategory == Tool::ECategory::EditPath;

	auto pathLambda = [&]( bool selected, bool selection, auto &view ) {
		mWireframePrimitiveShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
		mWireframePrimitiveShader->Apply( inDeviceContext );

		mWireframePrimitiveBatch->Begin();

		auto draw = [&]( const entt::entity entity ) {
			const auto &position = view.get<Position>( entity ).mValue;
			const auto &rotation = view.get<Rotation>( entity ).mPitchYawRoll;
			const PathCache &pathCache = view.get<PathCache>( entity );
			const PathData &pathData = view.get<PathData>( entity );
			const PathSelection &pathSelection = cregistry.get<PathSelection>( entity );

			DirectX::XMMATRIX rotmatF = DirectX::XMMatrixRotationRollPitchYawFromVector( rotation );
			Matrix44D rotmatD = Matrix44D::sFromXMMATRIX( rotmatF );
			Vector4D rebasedEntityPosition = ( position - cameraP );

			uint32_t entityColorU32;
			if ( !editPathMode && entity == selectedEntity ) {
				entityColorU32 = editPathMode ? colSelection : colSelected;
			}
			else if ( !editPathMode && selection ) {
				entityColorU32 = colSelection;
			}
			else {
				const auto &entityType = view.get<EntityType>( entity );
				entityColorU32 = entityManager.GetEntityTypeColor( entityType );
			}
			DirectX::XMVECTOR entityColorO = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );
			DirectX::XMVECTOR entityColorV = entityColorO;

			std::vector<VertexPositionColorID> linePoints( pathCache.mArray.size() );
			std::vector<VertexPositionColorID> linePointsL( pathCache.mArray.size() );
			std::vector<VertexPositionColorID> linePointsR( pathCache.mArray.size() );
			std::vector<VertexPositionColorID> linePointsLU( pathCache.mArray.size() );
			std::vector<VertexPositionColorID> linePointsRU( pathCache.mArray.size() );
			std::vector<DirectX::XMVECTOR> lineColors( pathData.mKnots.size() );
			if ( editPathMode ) {
				for ( size_t i = 0; i < lineColors.size(); ++i ) {
					if ( selected && pathSelection.mCurrentKnot == i ) {
						lineColors[i] = Cyclone::Util::ColorU32ToXMVECTOR(Cyclone::Util::ColorU32(255, 255, 0, 255));
					}
					else if ( pathSelection.mSelectedKnots.contains( i ) ) {
						lineColors[i] = Cyclone::Util::ColorU32ToXMVECTOR( Cyclone::Util::ColorU32( 255, 128, 0, 255 ) );
					}
					else {
						lineColors[i] = entityColorO;
					}
				}
			}

			uint32_t prevSubdivisions = pathCache.mCumulativeSubdivisions.front();

			for ( size_t i = 0; i + 1 < pathData.mKnots.size(); ++i ) {
				uint32_t cumSubdivisions = pathCache.mCumulativeSubdivisions[i + 1];
				uint32_t numSubdivisions = cumSubdivisions - prevSubdivisions;

				for ( size_t t = 0; t < numSubdivisions; ++t ) {
					using namespace DirectX;

					size_t s = prevSubdivisions + t;

					if ( editPathMode ) {
						float factor = static_cast<float>( t ) / ( numSubdivisions - 1 );
						float f2s = 4 * factor * factor;
						float f2i = 2 - 2 * factor;
						f2i *= f2i;

						entityColorV = DirectX::XMVectorLerp( lineColors[i], lineColors[i + 1], factor < 0.5 ? ( 1 - std::sqrtf( 1.0 - f2s ) ) / 2 : ( std::sqrtf( 1.0 - f2i ) + 1 ) / 2 );
					}

					DirectX::XMVECTOR P = ( rotmatD.TransformCoord3Unit( pathCache.mArray[s].mPosition ) + rebasedEntityPosition ).ToXMVECTOR();
					DirectX::XMVECTOR PL = DirectX::XMVectorScale( DirectX::XMVector3TransformCoord( pathCache.mArray[s].mDeltaL, rotmatF ), 1.0f );
					DirectX::XMVECTOR PR = DirectX::XMVectorScale( DirectX::XMVector3TransformCoord( pathCache.mArray[s].mDeltaR, rotmatF ), 1.0f );
					DirectX::XMVECTOR PLU = DirectX::XMVectorScale( DirectX::XMVector3TransformCoord( pathCache.mArray[s].mDeltaLU, rotmatF ), 1.0f );
					DirectX::XMVECTOR PRU = DirectX::XMVectorScale( DirectX::XMVector3TransformCoord( pathCache.mArray[s].mDeltaRU, rotmatF ), 1.0f );

					DirectX::XMVECTOR A = P + PL;
					DirectX::XMVECTOR B = P + PR;
					DirectX::XMVECTOR C = P + PLU;
					DirectX::XMVECTOR D = P + PRU;

					uint16_t idx = editPathMode ? i + 2 * t / numSubdivisions : 0;

					linePoints[s] = { P, entityColorV, entity, idx };
					linePointsL[s] = { A, entityColorV, entity, idx };
					linePointsR[s] = { B, entityColorV, entity, idx };
					linePointsLU[s] = { C, entityColorV, entity, idx };
					linePointsRU[s] = { D, entityColorV, entity, idx };

					mWireframePrimitiveBatch->DrawLine( linePointsL[s], linePointsR[s] );
					mWireframePrimitiveBatch->DrawLine( linePointsL[s], linePointsLU[s] );
					mWireframePrimitiveBatch->DrawLine( linePointsR[s], linePointsRU[s] );
				}
				prevSubdivisions = cumSubdivisions;
			}

			mWireframePrimitiveBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePoints.data(), linePoints.size() );
			mWireframePrimitiveBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsL.data(), linePointsL.size() );
			mWireframePrimitiveBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsR.data(), linePointsR.size() );
			mWireframePrimitiveBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsLU.data(), linePointsLU.size() );
			mWireframePrimitiveBatch->Draw( D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, linePointsRU.data(), linePointsRU.size() );
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

		mWireframePrimitiveBatch->End();
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

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( colSelected );
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
			auto view = cregistry.view<EntityType, Position, Rotation, PathCache, PathData, DrawTag, entt::tag<"is_selected"_hs>>();
			pathLambda( true, false, view );
		}

		inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 1 );
		{
			mWireframeBoxShader->Apply( inDeviceContext );
			mWireframeBoxShader->SetViewProj( inDeviceContext, viewMatrix, projMatrix );
			mWireframeBoxShader->SetMesh( inDeviceContext, inLevelInterface->GetPrimitives() );

			auto view = cregistry.view<Position, BoundingBox, DrawTag, entt::tag<"is_selected"_hs>>( entt::exclude<PathTag> );

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( colSelection );
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
			auto view = cregistry.view<EntityType, Position, Rotation, PathCache, PathData, DrawTag, entt::tag<"is_selected"_hs>>();
			pathLambda( false, true, view );
		}

		{
			if ( editPathMode ) {
				inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 1 );
			}
			else {
				inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 0 );
			}
			auto view = cregistry.view<EntityType, Position, Rotation, PathCache, PathData, DrawTag>( entt::exclude<entt::tag<"is_selected"_hs>> );
			pathLambda( false, false, view );
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
	}

	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );
	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), viewMatrix, projMatrix );
	mWireframeGridEffect->Apply( inDeviceContext );

	// Call all tool renders with depth enabled
	inDeviceContext->OMSetDepthStencilState( mLayeredDepthState.Get(), 0 );
	{
		mWireframeGridBatch->Begin();
		for ( auto &tool : inTools.mTools ) {
			tool->OnRender( T, inLevelInterface, mViewportData, mWireframeGridBatch.get() );
		}
		mWireframeGridBatch->End();

	}
}

template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::Perspective>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const Tool::ToolChanger & );
template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::TopXZ>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const Tool::ToolChanger & );
template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::FrontXY>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const Tool::ToolChanger & );
template void Cyclone::UI::ViewportElement::Render<Cyclone::UI::EViewportType::SideYZ>( ID3D11DeviceContext3 *, Cyclone::Core::LevelInterface *, const Tool::ToolChanger & );
