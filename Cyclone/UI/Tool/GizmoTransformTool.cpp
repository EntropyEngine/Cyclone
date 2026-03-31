#include "pch.h"
#include "Cyclone/UI/Tool/GizmoTransformTool.hpp"

// Math
#include "Cyclone/Math/Matrix.hpp"

// Core includes
#include "Cyclone/Core/LevelInterface.hpp"

// Compontnts
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"
#include "Cyclone/Core/Component/LocalBounds.hpp"

// DX includes
#include <DebugDraw.h>

// ImGuizmo
#include <ImGuizmo/ImGuizmo.h>

// STL
#include <bit>
#include <format>

using Cyclone::Math::Vector4D;
using Cyclone::Math::Matrix44D;
using Cyclone::Core::Component::Position;
using Cyclone::Core::Component::BoundingBox;
using Cyclone::Core::Component::Rotation;
using Cyclone::Core::Component::LocalBounds;
using Cyclone::Core::Tool::GizmoToolContext;

using TranslateInput = Cyclone::UI::Tool::GizmoTransformTool::TranslateInput;
using TranslateOutput = Cyclone::UI::Tool::GizmoTransformTool::TranslateOutput;

const bool IS_LOCAL = false;

namespace
{
	ImGuizmo::OPERATION TranslationOpFromAxis( size_t inAxis )
	{
		const ImGuizmo::OPERATION ops[] = { ImGuizmo::OPERATION::TRANSLATE_X, ImGuizmo::OPERATION::TRANSLATE_Y, ImGuizmo::OPERATION::TRANSLATE_Z };
		return ops[inAxis];
	}

	uint32_t MoveAxisFromMoveType( ImGuizmo::MOVETYPE inMoveType )
	{
		switch ( inMoveType ) {
			case ImGuizmo::MT_MOVE_X: return GizmoToolContext::XAxis;
			case ImGuizmo::MT_MOVE_Y: return GizmoToolContext::YAxis;
			case ImGuizmo::MT_MOVE_Z: return GizmoToolContext::ZAxis;
			case ImGuizmo::MT_MOVE_YZ: return GizmoToolContext::YAxis | GizmoToolContext::ZAxis;
			case ImGuizmo::MT_MOVE_ZX: return GizmoToolContext::XAxis | GizmoToolContext::ZAxis;
			case ImGuizmo::MT_MOVE_XY: return GizmoToolContext::XAxis | GizmoToolContext::YAxis;
			case ImGuizmo::MT_MOVE_SCREEN: return GizmoToolContext::XAxis | GizmoToolContext::YAxis | GizmoToolContext::ZAxis;
			default:
				assert( false );
				__assume( false );
		}
	}

	uint32_t RotateAxisFromMoveType( ImGuizmo::MOVETYPE inMoveType )
	{
		switch ( inMoveType ) {
			case ImGuizmo::MT_ROTATE_X: return GizmoToolContext::XAxis;
			case ImGuizmo::MT_ROTATE_Y: return GizmoToolContext::YAxis;
			case ImGuizmo::MT_ROTATE_Z: return GizmoToolContext::ZAxis;
			case ImGuizmo::MT_ROTATE_SCREEN: return GizmoToolContext::XAxis | GizmoToolContext::YAxis | GizmoToolContext::ZAxis;
			default:
				assert( false );
				__assume( false );
		}
	}

	void XM_CALLCONV Translate1( const TranslateInput &inInput, TranslateOutput &ioOutput, Vector4D inAxis1 )
	{
		ioOutput.mAxisMask = inAxis1;
		ioOutput.mAxisMaskInv = Vector4D::sReplicate( 1.0 ) - ioOutput.mAxisMask;

		Vector4D AP = inInput.mCameraPos - inInput.mOriginalPos;

		Vector4D cameraProjectedP = inInput.mOriginalPos + inAxis1 * Vector4D::sReplicate( AP.Dot3( inAxis1 ) ); // Todo: can we reduce lane switches?
		Vector4D diffP = ( inInput.mCameraPos - cameraProjectedP );
		Vector4D diffP2 = diffP + diffP;
		Vector4D deltaP = diffP.GetNorm3();

		Vector4D axisDir2 = Vector4D::sCross3( inAxis1, deltaP );

		Vector4D planeNormal = Vector4D::sCross3( inAxis1, axisDir2 );
		double planeCoordW = -planeNormal.Dot3( inInput.mOriginalPos );

		Vector4D mouseDelta = inInput.mMousePosFar - inInput.mMousePosNear;

		double planeV1 = planeNormal.Dot3( inInput.mMousePosNear );
		double planeV2 = planeNormal.Dot3( inInput.mMousePosFar );
		double planeD = planeV1 - planeV2;
		double planeVT = ( planeV1 + planeCoordW ) / planeD;

		Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
		Vector4D planeIntersection = intersectionDir + inInput.mMousePosNear;

		double flip = planeVT;

		// Inverted plane
		if ( flip < 0 ) {
			double planeCoordW2 = -planeNormal.Dot3( inInput.mOriginalPos + diffP2 );
			double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
			Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
			Vector4D planeIntersection2 = intersectionDir2 + inInput.mMousePosNear;

			ioOutput.mMousePos = planeIntersection2 - diffP2;
		}
		else if ( std::isfinite( flip ) ) {
			ioOutput.mMousePos = planeIntersection;
		}

		Vector4D deltaM = ioOutput.mMousePos - inInput.mOriginalPos;
		ioOutput.mMousePos = inInput.mOriginalPos + inAxis1 * Vector4D::sReplicate( inAxis1.Dot3( deltaM ) );
	}

	void XM_CALLCONV Translate2( const TranslateInput &inInput, TranslateOutput &ioOutput, Vector4D inAxis1, Vector4D inAxis2 )
	{
		ioOutput.mAxisMask = inAxis1 + inAxis2;
		ioOutput.mAxisMaskInv = Vector4D::sReplicate( 1.0 ) - ioOutput.mAxisMask;

		Vector4D planeNormal = Vector4D::sCross3( inAxis1, inAxis2 );
		double planeCoordW = -planeNormal.Dot3( inInput.mOriginalPos );

		Vector4D cameraProjectedP = inInput.mCameraPos - planeNormal * Vector4D::sReplicate( inInput.mCameraPos.Dot3( planeNormal ) - planeCoordW );
		Vector4D diffP = ( inInput.mCameraPos - cameraProjectedP );
		Vector4D diffP2 = diffP + diffP;

		Vector4D mouseDelta = inInput.mMousePosFar - inInput.mMousePosNear;

		double planeV1 = planeNormal.Dot3( inInput.mMousePosNear );
		double planeV2 = planeNormal.Dot3( inInput.mMousePosFar );
		double planeD = planeV1 - planeV2;
		double planeVT = ( planeV1 + planeCoordW ) / planeD;

		Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
		Vector4D planeIntersection = intersectionDir + inInput.mMousePosNear;

		double flip = planeVT;

		// Inverted plane
		if ( flip < 0 ) {
			double planeCoordW2 = -planeNormal.Dot3( inInput.mOriginalPos + diffP2 );
			double planeVT2 = ( planeV1 + planeCoordW2 ) / planeD;
			Vector4D intersectionDir2 = mouseDelta * Vector4D::sReplicate( planeVT2 );
			Vector4D planeIntersection2 = intersectionDir2 + inInput.mMousePosNear;

			ioOutput.mMousePos = planeIntersection2 - diffP2;
		}
		else if ( std::isfinite( flip ) ) {
			ioOutput.mMousePos = planeIntersection;
		}
	}

	void XM_CALLCONV Translate3( const TranslateInput &inInput, TranslateOutput &ioOutput, Vector4D inAxis1, Vector4D inAxis2 )
	{
		Vector4D planeNormal = Vector4D::sCross3( inAxis1, inAxis2 );
		double planeCoordW = -planeNormal.Dot3( inInput.mOriginalPos );

		ioOutput.mAxisMask = Vector4D::sReplicate( 1 );
		ioOutput.mAxisMaskInv = Vector4D::sZero();

		Vector4D mouseDelta = inInput.mMousePosFar - inInput.mMousePosNear;

		double planeV1 = planeNormal.Dot3( inInput.mMousePosNear );
		double planeV2 = planeNormal.Dot3( inInput.mMousePosFar );
		double planeD = planeV1 - planeV2;
		double planeVT = ( planeV1 + planeCoordW ) / planeD;

		Vector4D intersectionDir = mouseDelta * Vector4D::sReplicate( planeVT );
		Vector4D planeIntersection = intersectionDir + inInput.mMousePosNear;

		double flip = planeVT; // mouseDir.Dot3( intersectionDir.GetNorm3() );
		ioOutput.mMousePos = planeIntersection;
		if ( flip < 0 ) {
			assert( false );
		}
	}

	DirectX::XMVECTOR XM_CALLCONV QuatToPitchYawRoll( DirectX::XMVECTORF32 inQuat )
	{
		const float x = inQuat.f[0];
		const float y = inQuat.f[1];
		const float z = inQuat.f[2];
		const float w = inQuat.f[3];

		const float xx = x * x;
		const float yy = y * y;
		const float zz = z * z;

		const float m31 = 2.f * x * z + 2.f * y * w;
		const float m32 = 2.f * y * z - 2.f * x * w;
		const float m33 = 1.f - 2.f * xx - 2.f * yy;

		const float cy = sqrtf( m33 * m33 + m31 * m31 );
		const float cx = atan2f( -m32, cy );
		if (cy > 16.f * FLT_EPSILON)
		{
			const float m12 = 2.f * x * y + 2.f * z * w;
			const float m22 = 1.f - 2.f * xx - 2.f * zz;

			return DirectX::XMVectorSet( cx, atan2f( m31, m33 ), atan2f( m12, m22 ), 0.0f );
		}
		else
		{
			const float m11 = 1.f - 2.f * yy - 2.f * zz;
			const float m21 = 2.f * x * y - 2.f * z * w;

			return DirectX::XMVectorSet( cx, 0.f, atan2f( -m21, m11 ), 0.0f );
		}
	}
}

void Cyclone::UI::Tool::GizmoTransformTool::OnUpdate( EViewportType inType, Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	if ( mIsSelected ) {
		inViewportData.mDrawList->ChannelsSetCurrent( 3 );

		if ( inLevelInterface->GetGizmoCtx().mTransformType == GizmoToolContext::ETransformType::Translate ) {
			switch ( inType ) {
				case EViewportType::TopXZ: UpdateTranslateOrthographic<EViewportType::TopXZ>( inLevelInterface, inViewportData ); break;
				case EViewportType::FrontXY: UpdateTranslateOrthographic<EViewportType::FrontXY>( inLevelInterface, inViewportData ); break;
				case EViewportType::SideYZ: UpdateTranslateOrthographic<EViewportType::SideYZ>( inLevelInterface, inViewportData ); break;
				case EViewportType::Perspective: UpdateTranslatePerspective( inLevelInterface, inViewportData ); break;
			}
		}

		if ( inLevelInterface->GetGizmoCtx().mTransformType == GizmoToolContext::ETransformType::Rotate ) {
			switch ( inType ) {
				case EViewportType::TopXZ: UpdateRotateOrthographic<EViewportType::TopXZ>( inLevelInterface, inViewportData ); break;
				case EViewportType::FrontXY: UpdateRotateOrthographic<EViewportType::FrontXY>( inLevelInterface, inViewportData ); break;
				case EViewportType::SideYZ: UpdateRotateOrthographic<EViewportType::SideYZ>( inLevelInterface, inViewportData ); break;
				case EViewportType::Perspective: UpdateRotatePerspective( inLevelInterface, inViewportData ); break;
			}
		}
	}
}

void Cyclone::UI::Tool::GizmoTransformTool::OnShortcut( Cyclone::Core::LevelInterface * inLevelInterface )
{
	if ( mIsSelected && ImGui::IsKeyChordPressed( ImGuiKey_Tab ) ) inLevelInterface->GetGizmoCtx().mTransformType = static_cast<GizmoToolContext::ETransformType>( ( static_cast<int>( inLevelInterface->GetGizmoCtx().mTransformType ) + 1 ) % static_cast<int>( GizmoToolContext::ETransformType::COUNT ) );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::GizmoTransformTool::UpdateTranslateOrthographic( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	static constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	static constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &entityManager = inLevelInterface->GetEntityManager();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;


	if ( mIsSelected && selectedEntity != entt::null ) {
		Vector4D cameraP = orthographicContext.mCenter2D;
		Vector4D entityCurrentPosition = registry.get<Position>( selectedEntity ).mValue;
		Vector4D entityCurrentPositionRel = entityCurrentPosition - cameraP;

		ImGuizmo::SetGizmoSizeClipSpace( 128.0f / std::max( inViewportData.mViewSize.x, inViewportData.mViewSize.y ) );

		ImGuizmo::PushID( GetTypedID<T>( selectedEntity, ImGuizmo::MT_MOVE_X ) );
		ImGuizmo::SetRect( inViewportData.mViewOrigin.x, inViewportData.mViewOrigin.y, inViewportData.mViewSize.x, inViewportData.mViewSize.y );
		ImGuizmo::SetDrawlist( inViewportData.mDrawList );
		ImGuizmo::SetOrthographic( true );
		ImGuizmo::Enable( true );
		ImGuizmo::AllowAxisFlip( false );

		DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixTranslationFromVector( entityCurrentPositionRel.ToXMVECTOR() );

		ImGuizmo::OPERATION translateOp = TranslationOpFromAxis( AxisU ) | TranslationOpFromAxis( AxisV );

		ImGuizmo::Manipulate( reinterpret_cast<const float *>( &viewMatrix ), reinterpret_cast<const float *>( &projMatrix ), translateOp, ImGuizmo::WORLD, reinterpret_cast<float *>( &modelMatrix ) );

		ImGuizmo::PopID();
	}

	ImGuizmo::PushID( GetTypedID<T>( selectedEntity, ImGuizmo::MT_MOVE_X ) );
	if ( mIsSelected && inViewportData.mIsActive && ImGuizmo::IsUsing() && selectedEntity != entt::null ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			assert( gizmoContext.mActiveEntity == entt::null && "Active entity already set!" );

			entityManager.BeginAction();

			gizmoContext.mCurrentAxis = MoveAxisFromMoveType( ImGuizmo::GetMoveType() );
			gizmoContext.mInitialEntityPosition = registry.get<Position>( selectedEntity ).mValue;
			gizmoContext.mActiveEntity = selectedEntity;
		}

		assert( gizmoContext.mActiveEntity == selectedEntity && "Active entity missmatch!" );

		Vector4D entityOriginalPos = gizmoContext.mInitialEntityPosition;


		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		ImGuiIO &io = ImGui::GetIO();

		ImVec2 viewportAbsMousePos( io.MousePos.x - inViewportData.mViewOrigin.x, io.MousePos.y - inViewportData.mViewOrigin.y );
		ImVec2 viewportRelMousePos( viewportAbsMousePos.x - inViewportData.mViewSize.x / 2.0f, viewportAbsMousePos.y - inViewportData.mViewSize.y / 2.0f );
		float screenMouseU = viewportRelMousePos.x / ( inViewportData.mViewSize.x / 2.0f );
		float screenMouseV = -viewportRelMousePos.y / ( inViewportData.mViewSize.y / 2.0f );

		Vector4D cameraP = orthographicContext.mCenter2D;
		Vector4D mousePosNear = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( screenMouseU, screenMouseV, 0.0f, 0.0f ), viewProjInverse ) ) + cameraP;
		Vector4D mousePosFar = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( screenMouseU, screenMouseV, 0.99f, 0.0f ), viewProjInverse ) ) + cameraP;

		TranslateInput input{
			.mOriginalPos = entityOriginalPos,
			.mCameraPos = cameraP,
			.mMousePosNear = mousePosNear,
			.mMousePosFar = mousePosFar
		};

		TranslateOutput output;
		output.mMousePos = gizmoContext.mInitialMousePosition;

		int axisBitcount = std::popcount( gizmoContext.mCurrentAxis );

		// Always use screen space translation
		Translate3( input, output, Vector4D::sZeroSetValueByIndex<AxisU>( 1.0f ), Vector4D::sZeroSetValueByIndex<AxisV>( 1.0f ) );

		// Single axis transform
		if ( axisBitcount == 1 ) {
			Vector4D axisDir1{ nullptr };
			gizmoContext.GetSingleAxis( axisDir1 );

			output.mAxisMask = axisDir1;
			output.mAxisMaskInv = Vector4D::sReplicate( 1.0 ) - output.mAxisMask;
		}
		// Two axis transform
		else if ( axisBitcount == 2 ) {
			Vector4D axisDir1{ nullptr };
			Vector4D axisDir2{ nullptr };
			gizmoContext.GetDualAxis( axisDir1, axisDir2 );

			output.mAxisMask = axisDir1 + axisDir2;
			output.mAxisMaskInv = Vector4D::sReplicate( 1.0 ) - output.mAxisMask;
		}
		else {
			assert( false );
			__assume( false );
		}

		UpdateTranslate( inLevelInterface, input, output );
	}
	else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && gizmoContext.mActiveEntity != entt::null ) {
		for ( const entt::entity entity : selectedEntities ) {
			entityManager.UpdateEntity( entity, registry );
		}
		entityManager.EndAction( registry );
		gizmoContext.Deactivate();
	}
	ImGuizmo::PopID();
}

void Cyclone::UI::Tool::GizmoTransformTool::UpdateTranslatePerspective( Cyclone::Core::LevelInterface *inLevelInterface, const ViewportData &inViewportData )
{
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &entityManager = inLevelInterface->GetEntityManager();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;


	if ( mIsSelected && selectedEntity != entt::null ) {
		Vector4D cameraP = perspectiveContext.mCenter3D;
		Vector4D entityCurrentPosition = registry.get<Position>( selectedEntity ).mValue;
		Vector4D entityCurrentPositionRel = entityCurrentPosition - cameraP;

		ImGuizmo::SetGizmoSizeClipSpace( 128.0f / std::max( inViewportData.mViewSize.x, inViewportData.mViewSize.y ) );

		ImGuizmo::PushID( GetTypedID<EViewportType::Perspective>( selectedEntity, ImGuizmo::MT_MOVE_X ) );
		ImGuizmo::SetRect( inViewportData.mViewOrigin.x, inViewportData.mViewOrigin.y, inViewportData.mViewSize.x, inViewportData.mViewSize.y );
		ImGuizmo::SetDrawlist( inViewportData.mDrawList );
		ImGuizmo::SetOrthographic( false );
		ImGuizmo::Enable( true );
		ImGuizmo::AllowAxisFlip( false );

		DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixRotationRollPitchYawFromVector( registry.get<Rotation>( selectedEntity ).mPitchYawRoll ),
			DirectX::XMMatrixTranslationFromVector( entityCurrentPositionRel.ToXMVECTOR() )
		);

		ImGuizmo::Manipulate( reinterpret_cast<const float *>( &viewMatrix ), reinterpret_cast<const float *>( &projMatrix ), ImGuizmo::TRANSLATE, IS_LOCAL ? ImGuizmo::LOCAL : ImGuizmo::WORLD, reinterpret_cast<float *>( &modelMatrix ) );

		ImGuizmo::PopID();
	}
	
	ImGuizmo::PushID( GetTypedID<EViewportType::Perspective>( selectedEntity, ImGuizmo::MT_MOVE_X ) );
	if ( mIsSelected && inViewportData.mIsActive && ImGuizmo::IsUsing() && selectedEntity != entt::null ) {
		if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
			assert( gizmoContext.mActiveEntity == entt::null && "Active entity already set!" );

			entityManager.BeginAction();

			gizmoContext.mCurrentAxis = MoveAxisFromMoveType( ImGuizmo::GetMoveType() );
			gizmoContext.mInitialEntityPosition = registry.get<Position>( selectedEntity ).mValue;
			gizmoContext.mActiveEntity = selectedEntity;
		}

		assert( gizmoContext.mActiveEntity == selectedEntity && "Active entity missmatch!" );

		Vector4D entityOriginalPos = gizmoContext.mInitialEntityPosition;


		DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply( viewMatrix, projMatrix );
		DirectX::XMMATRIX viewProjInverse = DirectX::XMMatrixInverse( nullptr, viewProj );

		Vector4D cameraP = perspectiveContext.mCenter3D;
		Vector4D mousePosNear = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( static_cast<float>( inViewportData.mWorldMouseU ), static_cast<float>( inViewportData.mWorldMouseV ), 0.0f, 0.0f ), viewProjInverse ) ) + cameraP;
		Vector4D mousePosFar = Vector4D::sFromXMVECTOR( DirectX::XMVector3TransformCoord( DirectX::XMVectorSet( static_cast<float>( inViewportData.mWorldMouseU ), static_cast<float>( inViewportData.mWorldMouseV ), 0.99f, 0.0f ), viewProjInverse ) ) + cameraP;

		TranslateInput input{
			.mOriginalPos = entityOriginalPos,
			.mCameraPos = cameraP,
			.mMousePosNear = mousePosNear,
			.mMousePosFar = mousePosFar
		};

		TranslateOutput output;
		output.mMousePos = gizmoContext.mInitialMousePosition;

		int axisBitcount = std::popcount( gizmoContext.mCurrentAxis );

		// Single axis transform
		if ( axisBitcount == 1 ) {
			Vector4D axisDir1{ nullptr };
			gizmoContext.GetSingleAxis( axisDir1 );

			if ( IS_LOCAL ) {
				Matrix44D mat = Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( registry.get<Rotation>( selectedEntity ).mPitchYawRoll ) );
				axisDir1 = mat.TransformCoord3Unit( axisDir1 );
			}

			Translate1( input, output, axisDir1 );
		}
		// Two axis transform
		else if ( axisBitcount == 2 ) {
			Vector4D axisDir1{ nullptr };
			Vector4D axisDir2{ nullptr };
			gizmoContext.GetDualAxis( axisDir1, axisDir2 );

			if ( IS_LOCAL ) {
				Matrix44D mat = Matrix44D::sFromXMMATRIX( DirectX::XMMatrixRotationRollPitchYawFromVector( registry.get<Rotation>( selectedEntity ).mPitchYawRoll ) );
				axisDir1 = mat.TransformCoord3Unit( axisDir1 );
				axisDir2 = mat.TransformCoord3Unit( axisDir2 );
			}

			Translate2( input, output, axisDir1, axisDir2 );
		}
		// Camera aligned transform
		else if ( axisBitcount == 3 ) {
			DirectX::XMMATRIX viewRotation = DirectX::XMMatrixRotationRollPitchYaw( static_cast<float>( perspectiveContext.mCameraPitch ), static_cast<float>( perspectiveContext.mCameraYaw ), 0.0f );
			Vector4D axisDir1 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR0, viewRotation ) );
			Vector4D axisDir2 = Vector4D::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::g_XMIdentityR1, viewRotation ) );

			Translate3( input, output, axisDir1, axisDir2 );
		}
		else {
			assert( false );
			__assume( false );
		}

		UpdateTranslate( inLevelInterface, input, output );
	}
	else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && gizmoContext.mActiveEntity != entt::null ) {
		for ( const entt::entity entity : selectedEntities ) {
			entityManager.UpdateEntity( entity, registry );
		}
		entityManager.EndAction( registry );
		gizmoContext.Deactivate();
	}
	ImGuizmo::PopID();
}

void Cyclone::UI::Tool::GizmoTransformTool::UpdateTranslate( Cyclone::Core::LevelInterface * inLevelInterface, const TranslateInput &inInput, const TranslateOutput &inOutput )
{
	const auto &gridContext = inLevelInterface->GetGridCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &transformContext = inLevelInterface->GetSelectionTransformCtx();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto &selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	Vector4D objPos{ nullptr };
	if ( IS_LOCAL ) {
		objPos = inOutput.mMousePos + inInput.mOriginalPos;
	}
	else {
		objPos = inOutput.mMousePos * inOutput.mAxisMask + inInput.mOriginalPos * inOutput.mAxisMaskInv;
	}

	// If first frame of interactions, store initial position
	if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
		gizmoContext.mInitialMousePosition = objPos;
	}

	Vector4D objPosDelta = objPos - gizmoContext.mInitialMousePosition;

	Vector4D gridSize = Vector4D::sReplicate( gridContext.mGridSize );
	Vector4D gridSizeInv = Vector4D::sReplicate( 1.0 / gridContext.mGridSize );

	if ( gridContext.mSnapType == Cyclone::Core::Editor::GridContext::ESnapType::ByGrid ) {
		if ( IS_LOCAL ) {
			assert( false );
		}
		else {
			objPosDelta = objPosDelta * inOutput.mAxisMaskInv + Vector4D::sRound( objPosDelta * gridSizeInv ) * gridSize * inOutput.mAxisMask;
		}
	}

	Vector4D objPosNew = objPosDelta + inInput.mOriginalPos;
	objPosNew = Vector4D::sClamp( objPosNew, Vector4D::sReplicate( -gridContext.mWorldLimit ), Vector4D::sReplicate( gridContext.mWorldLimit ) );

	if ( gridContext.mSnapType == Cyclone::Core::Editor::GridContext::ESnapType::ToGrid ) {
		if ( IS_LOCAL ) {
			//objPosNew = Vector4D::sRound( objPosNew * gridSizeInv ) * gridSize;
			assert( false );
		}
		else {
			objPosNew = objPosNew * inOutput.mAxisMaskInv + Vector4D::sRound( objPosNew * gridSizeInv ) * gridSize * inOutput.mAxisMask;
		}
	}

	Vector4D perFrameDelta = objPosNew - registry.get<Position>( selectedEntity ).mValue;

	for ( const entt::entity entity : selectedEntities ) {
		registry.patch<Position>( entity, [perFrameDelta]( Position &inPosition ) { inPosition.mValue += perFrameDelta; } );
	}
	transformContext.UpdateOnDrag( perFrameDelta );
}


template<Cyclone::UI::EViewportType T>
void Cyclone::UI::Tool::GizmoTransformTool::UpdateRotateOrthographic( Cyclone::Core::LevelInterface * inLevelInterface, const ViewportData & inViewportData )
{
	size_t AxisW = ViewportTypeTraits<T>::AxisW;

	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &entityManager = inLevelInterface->GetEntityManager();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;


	if ( mIsSelected && selectedEntity != entt::null ) {
		Vector4D cameraP = orthographicContext.mCenter2D;
		Vector4D entityCurrentPosition = registry.get<Position>( selectedEntity ).mValue;
		Vector4D entityCurrentPositionRel = entityCurrentPosition - cameraP;

		ImGuizmo::SetGizmoSizeClipSpace( 128.0f / std::max( inViewportData.mViewSize.x, inViewportData.mViewSize.y ) );

		ImGuizmo::PushID( GetTypedID<T>( selectedEntity, ImGuizmo::MT_ROTATE_X ) );
		ImGuizmo::SetRect( inViewportData.mViewOrigin.x, inViewportData.mViewOrigin.y, inViewportData.mViewSize.x, inViewportData.mViewSize.y );
		ImGuizmo::SetDrawlist( inViewportData.mDrawList );
		ImGuizmo::SetOrthographic( true );
		ImGuizmo::Enable( true );
		ImGuizmo::AllowAxisFlip( false );

		DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixRotationRollPitchYawFromVector( registry.get<Rotation>( selectedEntity ).mPitchYawRoll ),
			DirectX::XMMatrixTranslationFromVector( entityCurrentPositionRel.ToXMVECTOR() )
		);

		ImGuizmo::OPERATION rotateOp[3] = {ImGuizmo::ROTATE_X, ImGuizmo::ROTATE_Y, ImGuizmo::ROTATE_Z };

		float snap = 1;
		//ImGuizmo::Manipulate( reinterpret_cast<const float *>( &viewMatrix ), reinterpret_cast<const float *>( &projMatrix ), rotateOp[AxisW], ImGuizmo::WORLD, reinterpret_cast<float *>( &modelMatrix ), nullptr, &snap );
		ImGuizmo::Manipulate( reinterpret_cast<const float *>( &viewMatrix ), reinterpret_cast<const float *>( &projMatrix ), ImGuizmo::ROTATE, ImGuizmo::LOCAL, reinterpret_cast<float *>( &modelMatrix ), nullptr, &snap );


		if ( inViewportData.mIsActive && ImGuizmo::IsUsing() ) {
			if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				assert( gizmoContext.mActiveEntity == entt::null && "Active entity already set!" );

				entityManager.BeginAction();

				gizmoContext.mCurrentAxis = RotateAxisFromMoveType( ImGuizmo::GetMoveType() );
				gizmoContext.mInitialEntityPosition = registry.get<Position>( selectedEntity ).mValue;
				gizmoContext.mInitialEntityRotation = registry.get<Rotation>( selectedEntity ).mPitchYawRoll;
				gizmoContext.mActiveEntity = selectedEntity;
			}

			UpdateRotate( inLevelInterface, modelMatrix );
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && gizmoContext.mActiveEntity != entt::null ) {
			for ( const entt::entity entity : selectedEntities ) {
				entityManager.UpdateEntity( entity, registry );
			}
			entityManager.EndAction( registry );
			gizmoContext.Deactivate();
		}
		ImGuizmo::PopID();
	}
}

void Cyclone::UI::Tool::GizmoTransformTool::UpdateRotatePerspective( Cyclone::Core::LevelInterface * inLevelInterface, const ViewportData & inViewportData )
{
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();

	auto &entityManager = inLevelInterface->GetEntityManager();

	auto &gizmoContext = inLevelInterface->GetGizmoCtx();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();
	entt::registry &registry = inLevelInterface->GetRegistry();

	const DirectX::XMMATRIX viewMatrix = inViewportData.mViewMatrix;
	const DirectX::XMMATRIX projMatrix = inViewportData.mProjMatrix;


	if ( mIsSelected && selectedEntity != entt::null ) {
		Vector4D cameraP = perspectiveContext.mCenter3D;
		Vector4D entityCurrentPosition = registry.get<Position>( selectedEntity ).mValue;
		Vector4D entityCurrentPositionRel = entityCurrentPosition - cameraP;

		ImGuizmo::SetGizmoSizeClipSpace( 128.0f / std::max( inViewportData.mViewSize.x, inViewportData.mViewSize.y ) );

		ImGuizmo::PushID( GetTypedID<EViewportType::Perspective>( selectedEntity, ImGuizmo::MT_ROTATE_X ) );
		ImGuizmo::SetRect( inViewportData.mViewOrigin.x, inViewportData.mViewOrigin.y, inViewportData.mViewSize.x, inViewportData.mViewSize.y );
		ImGuizmo::SetDrawlist( inViewportData.mDrawList );
		ImGuizmo::SetOrthographic( false );
		ImGuizmo::Enable( true );
		ImGuizmo::AllowAxisFlip( false );

		DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixRotationRollPitchYawFromVector( registry.get<Rotation>( selectedEntity ).mPitchYawRoll ),
			DirectX::XMMatrixTranslationFromVector( entityCurrentPositionRel.ToXMVECTOR() )
		);

		//float snap = 15;
		ImGuizmo::Manipulate( reinterpret_cast<const float *>( &viewMatrix ), reinterpret_cast<const float *>( &projMatrix ), ImGuizmo::ROTATE, ImGuizmo::LOCAL, reinterpret_cast<float *>( &modelMatrix ), nullptr );


		if ( inViewportData.mIsActive && ImGuizmo::IsUsing() ) {
			if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
				assert( gizmoContext.mActiveEntity == entt::null && "Active entity already set!" );

				entityManager.BeginAction();

				gizmoContext.mCurrentAxis = RotateAxisFromMoveType( ImGuizmo::GetMoveType() );
				gizmoContext.mInitialEntityPosition = registry.get<Position>( selectedEntity ).mValue;
				gizmoContext.mInitialEntityRotation = registry.get<Rotation>( selectedEntity ).mPitchYawRoll;
				gizmoContext.mActiveEntity = selectedEntity;
			}

			UpdateRotate( inLevelInterface, modelMatrix );
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) && gizmoContext.mActiveEntity != entt::null ) {
			for ( const entt::entity entity : selectedEntities ) {
				entityManager.UpdateEntity( entity, registry );
			}
			entityManager.EndAction( registry );
			gizmoContext.Deactivate();
		}
		ImGuizmo::PopID();
	}
}

void XM_CALLCONV Cyclone::UI::Tool::GizmoTransformTool::UpdateRotate( Cyclone::Core::LevelInterface *inLevelInterface, DirectX::FXMMATRIX inModelMatrix )
{
	const auto &selectionContext = inLevelInterface->GetSelectionCtx();
	auto &gizmoContext = inLevelInterface->GetGizmoCtx();
	entt::registry &registry = inLevelInterface->GetRegistry();

	entt::entity selectedEntity = selectionContext.GetSelectedEntity();
	const auto & selectedEntities = selectionContext.GetSelectedEntities();

	DirectX::XMVECTORF32 rotationQuat;
	DirectX::XMVECTORF32 scale;
	DirectX::XMVECTORF32 translation;

	DirectX::XMMatrixDecompose( &scale.v, &rotationQuat.v, &translation.v, inModelMatrix );

	DirectX::XMVECTOR newQuat = rotationQuat;
	DirectX::XMVECTOR origQuat = DirectX::XMQuaternionRotationRollPitchYawFromVector( registry.get<Rotation>( selectedEntity ).mPitchYawRoll );

	DirectX::XMVECTOR deltaQuat = DirectX::XMQuaternionMultiply( DirectX::XMQuaternionInverse( origQuat ), newQuat );

	registry.get<Rotation>( selectedEntity ).mPitchYawRoll = QuatToPitchYawRoll( rotationQuat );
	registry.get<LocalBounds>( selectedEntity ).UpdateBoundingBox( selectedEntity, registry );

	DirectX::XMMATRIX deltaMatrix = DirectX::XMMatrixRotationQuaternion( deltaQuat );

	Matrix44D deltaMatrixD = Matrix44D::sFromXMMATRIX( deltaMatrix );

	for ( entt::entity entity : selectedEntities ) {
		if ( entity != selectedEntity ) {
			auto &currP = registry.get<Position>( entity );
			auto &currR = registry.get<Rotation>( entity );

			Vector4D deltaP = currP.mValue - gizmoContext.mInitialEntityPosition;
			Vector4D deltaPResult = deltaMatrixD.TransformCoord3Unit( deltaP );

			currP.mValue += deltaPResult - deltaP;

			DirectX::XMVECTOR currRQ = DirectX::XMQuaternionRotationRollPitchYawFromVector( currR.mPitchYawRoll );
			DirectX::XMVECTOR newRQ = DirectX::XMQuaternionMultiply( deltaQuat, currRQ );
			currR.mPitchYawRoll = QuatToPitchYawRoll( { .v = newRQ } );

			registry.get<LocalBounds>( entity ).UpdateBoundingBox( entity, registry );
		}
	}
}