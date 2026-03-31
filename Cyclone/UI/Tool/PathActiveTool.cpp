#include "pch.h"
#include "Cyclone/UI/Tool/PathActiveTool.hpp"

// Math
#include "Cyclone/Math/Matrix.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Compontnts
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// DX includes
#include <DebugDraw.h>

using Cyclone::Math::Vector4D;
using Cyclone::Math::Matrix44D;
using Cyclone::Core::Component::EntityType;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Component::Rotation;
using Cyclone::Core::Component::PathData;
using Cyclone::Core::Component::PathTag;

void Cyclone::UI::Tool::PathActiveTool::OnRender( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	if ( mIsSelected ) {
		switch ( inType ) {
			case EViewportType::Perspective: OnRender<EViewportType::Perspective>( inLevelInterface, inViewportData, inPrimitiveBatch ); break;
			case EViewportType::TopXZ: OnRender<EViewportType::TopXZ>( inLevelInterface, inViewportData, inPrimitiveBatch ); break;
			case EViewportType::FrontXY: OnRender<EViewportType::FrontXY>( inLevelInterface, inViewportData, inPrimitiveBatch ); break;
			case EViewportType::SideYZ: OnRender<EViewportType::SideYZ>( inLevelInterface, inViewportData, inPrimitiveBatch ); break;
		}
	}
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::PathActiveTool::OnRender( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData, DrawType *inPrimitiveBatch )
{
	Vector4D cameraP = T == EViewportType::Perspective ? inLevelInterface->GetPerspectiveCtx().mCenter3D : inLevelInterface->GetOrthographicCtx().mCenter2D;

	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	const auto &entityManager = inLevelInterface->GetEntityManager();

	const entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const std::set<entt::entity> &selectedEntities = selectionContext.GetSelectedEntities();

	const entt::registry &cregistry = inLevelInterface->GetRegistry();

	DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;

	using DrawTag = ViewportTypeTraits<T>::DrawTag;
	auto view = cregistry.view<EntityType, Position, Rotation, PathTag, PathData, DrawTag>();
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

		for ( const auto &segment : pathData.mPathSegments ) {
			inPrimitiveBatch->DrawLine(
				{ ( rotmat.TransformCoord3Unit( segment.mP0 ) + rebasedEntityPosition ).ToXMVECTOR(), DirectX::Colors::White },
				{ ( rotmat.TransformCoord3Unit( segment.mP1 ) + rebasedEntityPosition ).ToXMVECTOR(), DirectX::Colors::White }
			);

			inPrimitiveBatch->DrawLine(
				{ ( rotmat.TransformCoord3Unit( segment.mP2 ) + rebasedEntityPosition ).ToXMVECTOR(), DirectX::Colors::White },
				{ ( rotmat.TransformCoord3Unit( segment.mP3 ) + rebasedEntityPosition ).ToXMVECTOR(), DirectX::Colors::White }
			);
		}
	}
}