#include "pch.h"
#include "Cyclone/UI/ViewportElementOrthographic.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// Cyclone utils
#include "Cyclone/Util/Render.hpp"

using Cyclone::Math::XLVector;

namespace
{
	template<Cyclone::UI::EViewportType T>
	DirectX::XMMATRIX XM_CALLCONV GetViewMatrix( double inWorldLimit );

	template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<Cyclone::UI::EViewportType::TopXZ>( double inWorldLimit )
	{
		return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( 0.0f, static_cast<float>( 2 * inWorldLimit ), 0.0f, 0.0f ), -DirectX::g_XMIdentityR1, DirectX::g_XMIdentityR2 );
	}

	template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<Cyclone::UI::EViewportType::FrontXY>( double inWorldLimit )
	{
		return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( 0.0f, 0.0f, static_cast<float>( -2 * inWorldLimit ), 0.0f ), DirectX::g_XMIdentityR2, DirectX::g_XMIdentityR1 );
	}

	template<> DirectX::XMMATRIX XM_CALLCONV GetViewMatrix<Cyclone::UI::EViewportType::SideYZ>( double inWorldLimit )
	{
		return DirectX::XMMatrixLookToRH( DirectX::XMVectorSet( static_cast<float>( 2 * inWorldLimit ), 0.0f, 0.0f, 0.0f ), -DirectX::g_XMIdentityR0, DirectX::g_XMIdentityR1 );
	}

	
	DirectX::XMMATRIX XM_CALLCONV GetProjMatrix( size_t inWidth, size_t inHeight, double inZoomScale2D, double inWorldLimit )
	{
		return DirectX::XMMatrixOrthographicRH( static_cast<float>( inWidth * inZoomScale2D ), static_cast<float>( inHeight * inZoomScale2D ), 1.0f, static_cast<float>( 4 * inWorldLimit ) );
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface, ViewportGridContext& inGridContext, ViewportOrthographicContext& inOrthographicContext )
{

}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportElementOrthographic<T>::Render( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface, const ViewportGridContext &inGridContext, const ViewportOrthographicContext &inOrthographicContext )
{
	Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>( inGridContext.mWorldLimit ), GetProjMatrix( mWidth, mHeight, inOrthographicContext.mZoomScale2D, inGridContext.mWorldLimit ) );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		constexpr size_t AxisU = ViewportElementOrthographic::AxisU;
		constexpr size_t AxisV = ViewportElementOrthographic::AxisV;

		double minU, maxU, minV, maxV;
		GetMinMaxUV( inOrthographicContext.mCenter2D, inGridContext.mWorldLimit, inOrthographicContext.mZoomScale2D, minU, maxU, minV, maxV );

		double subgridStep = inGridContext.mGridSize;
		double gridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;

		while ( subgridStep / inOrthographicContext.mZoomScale2D < kMinGridSize ) {
			//subgridStep *= 10;
			subgridStep = std::pow( 10.0, std::ceil( std::log10( subgridStep * 4 ) ) ) / 2;
		}

		while ( gridStep / inOrthographicContext.mZoomScale2D < kMinGridSize * 5 ) {
			//gridStep *= 10;
			gridStep = std::pow( 10.0, std::ceil( std::log10( gridStep * 4 ) ) ) / 2;
		}

		if ( subgridStep / inOrthographicContext.mZoomScale2D > kMinGridSize ) {
			DrawLineLoop<AxisU, AxisV>( inOrthographicContext.mCenter2D, minU, maxU, minV, maxV, subgridStep, DirectX::ColorsLinear::DimGray );
			DrawLineLoop<AxisV, AxisU>( inOrthographicContext.mCenter2D, minV, maxV, minU, maxU, subgridStep, DirectX::ColorsLinear::DimGray );
		}

		DrawLineLoop<AxisU, AxisV>( inOrthographicContext.mCenter2D, minU, maxU, minV, maxV, gridStep, DirectX::Colors::DimGray );
		DrawLineLoop<AxisV, AxisU>( inOrthographicContext.mCenter2D, minV, maxV, minU, maxU, gridStep, DirectX::Colors::DimGray );

		DrawLineLoop<AxisU, AxisV>( inOrthographicContext.mCenter2D, minU, maxU, minV, maxV, 1000, DirectX::Colors::Gray );
		DrawLineLoop<AxisV, AxisU>( inOrthographicContext.mCenter2D, minV, maxV, minU, maxU, 1000, DirectX::Colors::Gray );

		DrawAxisLine<AxisU>( inOrthographicContext.mCenter2D, minU, maxU );
		DrawAxisLine<AxisV>( inOrthographicContext.mCenter2D, minV, maxV );
	}
	mWireframeGridBatch->End();


	// Switch to depth buffer
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthDefault(), 0 );

	mWireframeGridBatch->Begin();
	{
		// Iterate over all entities
		const entt::registry &cregistry = inLevelInterface->GetRegistry();
		auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>();
		for ( const entt::entity entity : view ) {

			const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
			const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
			const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity );

			bool entityInSelection = inLevelInterface->GetSelectedEntities().contains( entity );
			bool entityIsSelected = inLevelInterface->GetSelectedEntity() == entity;

			uint32_t entityColorU32;
			if ( entityIsSelected ) {
				entityColorU32 = Cyclone::Util::ColorU32( 255, 255, 0, 255 );
			}
			else if ( entityInSelection ) {
				entityColorU32 = Cyclone::Util::ColorU32( 255, 128, 0, 255 );
			}
			else {
				entityColorU32 = entt::resolve( static_cast<entt::id_type>( entityType ) ).data( "debug_color"_hs ).get( {} ).cast<uint32_t>();
			}

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

			XLVector rebasedEntityPosition = ( position - inOrthographicContext.mCenter2D );
			XLVector rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			Cyclone::Util::RenderWireframeBoundingBox( mWireframeGridBatch.get(), rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}
	}
	mWireframeGridBatch->End();

	Resolve( inDeviceContext );
}

template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::TopXZ>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::FrontXY>;
template class Cyclone::UI::ViewportElementOrthographic<Cyclone::UI::EViewportType::SideYZ>;
