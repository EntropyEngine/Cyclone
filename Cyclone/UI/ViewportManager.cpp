#include "pch.h"
#include "Cyclone/UI/ViewportManager.hpp"

// Cyclone core includes
#include "Cyclone/Core/LevelInterface.hpp"
#include "Cyclone/Core/Entity/EntityTypeRegistry.hpp"

// Cyclone components
#include "Cyclone/Core/Component/EntityType.hpp"
#include "Cyclone/Core/Component/Position.hpp"
#include "Cyclone/Core/Component/BoundingBox.hpp"

// STL Includes
#include <format>

// ImGui Includes
#include <imgui.h>
#include <imgui_internal.h>

// DX Includes
#include <DirectXHelpers.h>

using Cyclone::Math::XLVector;

namespace
{
	void DrawViewportOverlay( const char *inText, float inPadding = 4 )
	{
		ImGui::SetCursorPos( { 0, 0 } );
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1 = ImGui::CalcTextSize( inText );
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( p0, { p0.x + p1.x + inPadding * 2, p0.y + p1.y + inPadding * 2 }, IM_COL32( 0, 0, 0, 128 ) );

		ImGui::SetCursorPos( { inPadding, inPadding } );
		ImGui::Text( inText );
	}

	void XM_CALLCONV DrawWireframeBoundingBox( DirectX::PrimitiveBatch<DirectX::VertexPositionColor> *inBatch, DirectX::FXMVECTOR inRebasedBoxCenter, DirectX::FXMVECTOR inBoxExtent, DirectX::FXMVECTOR inBoxColor )
	{
		DirectX::XMMATRIX matWorld = DirectX::XMMatrixScalingFromVector( inBoxExtent );
		matWorld.r[3] = DirectX::XMVectorSelect( matWorld.r[3], inRebasedBoxCenter, DirectX::g_XMSelect1110 );

		static const DirectX::XMVECTORF32 s_verts[8] =
		{
			{ { { -1.f, -1.f, -1.f, 0.f } } },
			{ { {  1.f, -1.f, -1.f, 0.f } } },
			{ { {  1.f, -1.f,  1.f, 0.f } } },
			{ { { -1.f, -1.f,  1.f, 0.f } } },
			{ { { -1.f,  1.f, -1.f, 0.f } } },
			{ { {  1.f,  1.f, -1.f, 0.f } } },
			{ { {  1.f,  1.f,  1.f, 0.f } } },
			{ { { -1.f,  1.f,  1.f, 0.f } } }
		};

		static const WORD s_indices[] =
		{
			0, 1,
			1, 2,
			2, 3,
			3, 0,
			4, 5,
			5, 6,
			6, 7,
			7, 4,
			0, 4,
			1, 5,
			2, 6,
			3, 7
		};

		DirectX::VertexPositionColor verts[8];

		for (size_t i = 0; i < 8; ++i) {
			DirectX::XMVECTOR v = DirectX::XMVector3Transform( s_verts[i], matWorld );
			DirectX::XMStoreFloat3( &verts[i].position, v );
			DirectX::XMStoreFloat4( &verts[i].color, inBoxColor );
		}

		inBatch->DrawIndexed( D3D_PRIMITIVE_TOPOLOGY_LINELIST, s_indices, static_cast<UINT>( std::size( s_indices ) ), verts, 8 );
	}

	void DrawCross( ImDrawList *inDrawList, const ImVec2 &inOrigin, float inWidth, ImU32 inColor )
	{
		inDrawList->AddLine( { inOrigin.x - inWidth, inOrigin.y - inWidth }, { inOrigin.x + inWidth, inOrigin.y + inWidth }, inColor );
		inDrawList->AddLine( { inOrigin.x + inWidth, inOrigin.y - inWidth }, { inOrigin.x - inWidth, inOrigin.y + inWidth }, inColor );
	}
}

Cyclone::UI::ViewportManager::ViewportManager()
{
	mViewportPerspective = std::make_unique<ViewportElement>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportTop = std::make_unique<ViewportElement>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportFront = std::make_unique<ViewportElement>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
	mViewportSide = std::make_unique<ViewportElement>( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, DirectX::Colors::Black );
}

void Cyclone::UI::ViewportManager::SetDevice( ID3D11Device3 *inDevice )
{
	mViewportPerspective->SetDevice( inDevice );
	mViewportTop->SetDevice( inDevice );
	mViewportFront->SetDevice( inDevice );
	mViewportSide->SetDevice( inDevice );

	mCommonStates = std::make_unique<DirectX::CommonStates>( inDevice );

	mWireframeGridEffect = std::make_unique<DirectX::BasicEffect>( inDevice );
	mWireframeGridEffect->SetVertexColorEnabled( true );

	DX::ThrowIfFailed( DirectX::CreateInputLayoutFromEffect<DirectX::VertexPositionColor>( inDevice, mWireframeGridEffect.get(), mWireframeGridInputLayout.ReleaseAndGetAddressOf() ) );

	Microsoft::WRL::ComPtr<ID3D11DeviceContext3> deviceContext;
	inDevice->GetImmediateContext3( deviceContext.GetAddressOf() );

	mWireframeGridBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>( deviceContext.Get() );
}

void Cyclone::UI::ViewportManager::MenuBarUpdate()
{
	if ( ImGui::MenuItem( "Autosize Viewports", "Ctrl+A") ) mShouldAutosize = true;
}

void Cyclone::UI::ViewportManager::ToolbarUpdate()
{
	ImVec2 viewSize = ImGui::GetWindowSize();

	ImGui::PushStyleVarX( ImGuiStyleVar_SelectableTextAlign, 1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 4.0f );

	const float textOffset = 6.0f;
	const float itemOffset = 3.0f;

	const float textSpacing = 5.0f;
	const float childSpacing = 8.0f;

	ImGui::SetCursorPos( { 5, 5 } );

	ImGui::BeginChild( "##GridSizeChunk", { 0, viewSize.y - 10 }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_FrameStyle );
	{
		ImGui::SetCursorPosY( textOffset );
		ImGui::Text( "Grid:" );

		ImGui::SameLine( 0.0f, textSpacing );

		ImGui::SetCursorPosY( itemOffset );
		ImGui::SetNextItemWidth( 52.0f );
		std::string subGridLevelPreview = std::format( "{}", kSubGridLevelText[mSubGridSizeIndex] );
		if ( ImGui::BeginCombo( "##SubGridLevel", subGridLevelPreview.c_str(), ImGuiComboFlags_HeightLarge ) ) {
			for ( int i = 0; i < std::size( kSubGridLevels ); ++i ) {
				const bool isSelected = mSubGridSizeIndex == i;
				if ( ImGui::Selectable( kSubGridLevelText[i], isSelected ) ) {
					mSubGridSizeIndex = i;
				}
				if ( isSelected ) ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		// Adjust grid size with [ and ]
		mSubGridSizeIndex -= ImGui::IsKeyPressed( ImGuiKey_LeftBracket, false );
		mSubGridSizeIndex += ImGui::IsKeyPressed( ImGuiKey_RightBracket, false );
		mSubGridSizeIndex = std::clamp( mSubGridSizeIndex, 0, static_cast<int>( std::size( kSubGridLevels ) ) - 1 );

		// Propogate any grid level changes to the actual value
		mSubGridSize = kSubGridLevels[mSubGridSizeIndex];
	}
	ImGui::EndChild();

	ImGui::SameLine( 0.0f, childSpacing );

	ImGui::BeginChild( "##GridSnapChunk", { 0, viewSize.y - 10 }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_FrameStyle );
	{
		ImGui::SetCursorPosY( textOffset );
		ImGui::Text( "Snap:" );

		ImGui::SameLine( 0.0f, textSpacing );

		ImGui::SetCursorPosY( itemOffset );
		if ( ImGui::RadioButton( "To Grid", mGridSnapType == EGridSnapType::ToGrid ) ) mGridSnapType = EGridSnapType::ToGrid;

		ImGui::SameLine();

		ImGui::SetCursorPosY( itemOffset );
		if ( ImGui::RadioButton( "By Grid", mGridSnapType == EGridSnapType::ByGrid ) ) mGridSnapType = EGridSnapType::ByGrid;

		ImGui::SameLine();

		ImGui::SetCursorPosY( itemOffset );
		if ( ImGui::RadioButton( "None ", mGridSnapType == EGridSnapType::None ) ) mGridSnapType = EGridSnapType::None;
		
	}
	ImGui::EndChild();

	ImGui::PopStyleVar( 2 );

	
}

void Cyclone::UI::ViewportManager::UpdatePerspective( float inDeltaTime )
{
	ImVec2 viewSize = ImGui::GetWindowSize();
	ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	const bool isHovered = ImGui::IsItemHovered();
	const bool isActive = ImGui::IsItemActive();

	if ( isActive && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
		mCameraPitch += io.MouseDelta.y * kMouseSensitivity;
		mCameraYaw -= io.MouseDelta.x * kMouseSensitivity;		

		float forward = 0.0f;
		float left = 0.0f;

		forward += ImGui::IsKeyDown( ImGuiKey_W );
		forward -= ImGui::IsKeyDown( ImGuiKey_S );
		left += ImGui::IsKeyDown( ImGuiKey_A );
		left -= ImGui::IsKeyDown( ImGuiKey_D );

		forward *= kKeyboardSensitivity * inDeltaTime;
		left *= kKeyboardSensitivity * inDeltaTime;

		if ( forward || left ) {
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw( mCameraPitch, mCameraYaw, 0.0f );
			mCenter3D += XLVector::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::XMVectorSet( left, 0, forward, 0 ), rotationMatrix ) );
		}
	}

	if ( isHovered ) {
		float scroll = io.MouseWheel;
		scroll *= kCameraDollySensitivity;
		if ( scroll ) {
			DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw( mCameraPitch, mCameraYaw, 0.0f );
			mCenter3D += XLVector::sFromXMVECTOR( DirectX::XMVector3Transform( DirectX::XMVectorSet( 0, 0, scroll, 0 ), rotationMatrix ) );
		}
	}

	constexpr float pitchLimit = DirectX::XM_PIDIV2 - 0.01f;
	mCameraPitch = std::clamp( mCameraPitch, -pitchLimit, pitchLimit );
	mCameraYaw = mCameraYaw - DirectX::XM_2PI * std::floor( mCameraYaw / DirectX::XM_2PI );
}

template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportManager::UpdateWireframe( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	ImVec2 viewSize = ImGui::GetWindowSize();
	ImVec2 viewOrigin = ImGui::GetCursorScreenPos();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::Image( GetViewport<T>()->GetOrResizeSRV( static_cast<size_t>( viewSize.x ), static_cast<size_t>( viewSize.y ) ), viewSize );

	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetCursorPos( { 0, 0 } );
	ImGui::SetNextItemAllowOverlap();
	ImGui::InvisibleButton( "canvas", viewSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle );
	const bool isCanvasHovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenOverlappedByItem );
	const bool isCanvasActive = ImGui::IsItemActive();

	ImVec2 viewportAbsMousePos( io.MousePos.x - viewOrigin.x, io.MousePos.y - viewOrigin.y );
	ImVec2 viewportRelMousePos( viewportAbsMousePos.x - viewSize.x / 2.0f, viewportAbsMousePos.y - viewSize.y / 2.0f );

	double worldMouseU = GetCenter2D<AxisU>() - viewportRelMousePos.x * mZoomScale2D;
	double worldMouseV = GetCenter2D<AxisV>() - viewportRelMousePos.y * mZoomScale2D;

	double worldSnapU = std::round( worldMouseU / mSubGridSize ) * mSubGridSize;
	double worldSnapV = std::round( worldMouseV / mSubGridSize ) * mSubGridSize;

	const char *uStr = std::array{ "x", "y", "z" }[AxisU];
	const char *vStr = std::array{ "x", "y", "z" }[AxisV];

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImGuiStyle().WindowPadding );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImGuiStyle().ItemSpacing );
	if ( isCanvasHovered ) ImGui::SetTooltip(
		"Mouse pos: (%.0f, %.0f)\n"
		"Viewport abs pos: (%.0f, %.0f)\n"
		"Viewport rel pos: (%.1f, %.1f)\n"
		"World Pos: (%s=%.2f, %s=%.2f)\n"
		"Snap Pos: (%s=%.2f, %s=%.2f)\n"
		"Zoom Level: (%.3f)",
		io.MousePos.x, io.MousePos.y,
		viewportAbsMousePos.x, viewportAbsMousePos.y,
		viewportRelMousePos.x, viewportRelMousePos.y,
		uStr, worldMouseU, vStr, worldMouseV,
		uStr, worldSnapU, vStr, worldSnapV,
		mZoomScale2D
	);
	ImGui::PopStyleVar( 2 );

	if ( isCanvasHovered && ImGui::IsMouseDragging( ImGuiMouseButton_Middle, 0.0f ) ) {
		mCenter2D += XLVector::sZeroSetValueByIndex<AxisU>( io.MouseDelta.x * mZoomScale2D );
		mCenter2D += XLVector::sZeroSetValueByIndex<AxisV>( io.MouseDelta.y * mZoomScale2D );
	}

	if ( isCanvasHovered && io.MouseWheel ) {
		mZoomLevel -= ( io.MouseWheel > 0 ) - ( io.MouseWheel < 0 );
		double newZoomScale2D = sZoomLevelToScale( mZoomLevel );

		double uPosNew = GetCenter2D<AxisU>() - viewportRelMousePos.x * newZoomScale2D;
		double vPosNew = GetCenter2D<AxisV>() - viewportRelMousePos.y * newZoomScale2D;

		mCenter2D += XLVector::sZeroSetValueByIndex<AxisU>( std::lerp( worldMouseU, worldSnapU, static_cast<double>( inDeltaTime * kAccelerateToSnap ) ) - uPosNew );
		mCenter2D += XLVector::sZeroSetValueByIndex<AxisV>( std::lerp( worldMouseV, worldSnapV, static_cast<double>( inDeltaTime * kAccelerateToSnap ) ) - vPosNew );

		mZoomScale2D = newZoomScale2D;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->Flags |= ImDrawListFlags_AntiAliasedLines;
	drawList->ChannelsSplit( 3 );
	drawList->ChannelsSetCurrent( 0 );

	if ( isCanvasHovered ) {
		ImVec2 gridPos;
		gridPos.x = static_cast<float>( ( GetCenter2D<AxisU>() - worldSnapU ) / mZoomScale2D + viewSize.x / 2.0f + viewOrigin.x );
		gridPos.y = static_cast<float>( ( GetCenter2D<AxisV>() - worldSnapV ) / mZoomScale2D + viewSize.y / 2.0f + viewOrigin.y );

		//float offset = static_cast<float>( std::max( 2.0, mSubGridSize / mZoomScale2D ) );
		float offset = static_cast<float>( 2.0f );
		DrawCross( drawList, gridPos, offset, IM_COL32( 255, 255, 255, 255 ) );
	}

	// Get smaller font for debug text
	ImFont* narrowFont = io.Fonts->Fonts[1];
	float fontSize = ImGui::GetFontSize();

	ImVec2 selectedBoxMin = { viewOrigin.x + viewSize.x, viewOrigin.y + viewSize.y };
	ImVec2 selectedBoxMax = viewOrigin;

	const double invZoom = 1.0 / mZoomScale2D;
	const float offsetX = viewSize.x / 2.0f + viewOrigin.x;
	const float offsetY = viewSize.y / 2.0f + viewOrigin.y;

	// Iterate over all entities
	entt::registry &registry = inLevelInterface->GetRegistry();
	const entt::registry &cregistry = inLevelInterface->GetRegistry();
	auto view = cregistry.view<Cyclone::Core::Component::EntityType, Cyclone::Core::Component::Position, Cyclone::Core::Component::BoundingBox>();
	for ( const entt::entity entity : view ) {

		const auto &entityType = view.get<Cyclone::Core::Component::EntityType>( entity );
		const auto &position = view.get<Cyclone::Core::Component::Position>( entity );
		const auto &boundingBox = view.get<Cyclone::Core::Component::BoundingBox>( entity );



		auto entityColor = entt::resolve( static_cast<entt::id_type>( entityType ) ).data( "debug_color"_hs ).get( {} ).cast<uint32_t>();

		bool entityInSelection = inLevelInterface->GetSelectedEntities().contains( entity );
		bool entityIsSelected = inLevelInterface->GetSelectedEntity() == entity;

		if ( entityIsSelected ) {
			entityColor = IM_COL32( 255, 255, 0, 255 );
			drawList->ChannelsSetCurrent( 2 );
		}
		else if ( entityInSelection ) {
			entityColor = IM_COL32( 255, 128, 0, 255 );
			drawList->ChannelsSetCurrent( 1 );
		}
		else {
			drawList->ChannelsSetCurrent( 0 );
		}

		XLVector rebasedEntityPosition = ( mCenter2D - position );
		XLVector rebasedBoundingBoxMin = rebasedEntityPosition - boundingBox.mCenter - boundingBox.mExtent;
		XLVector rebasedBoundingBoxMax = rebasedEntityPosition - boundingBox.mCenter + boundingBox.mExtent;

		ImVec2 localBoxMin;
		localBoxMin.x = static_cast<float>( rebasedBoundingBoxMin.Get<AxisU>() * invZoom + offsetX );
		localBoxMin.y = static_cast<float>( rebasedBoundingBoxMin.Get<AxisV>() * invZoom + offsetY );

		ImVec2 localBoxMax;
		localBoxMax.x = static_cast<float>( rebasedBoundingBoxMax.Get<AxisU>() * invZoom + offsetX );
		localBoxMax.y = static_cast<float>( rebasedBoundingBoxMax.Get<AxisV>() * invZoom + offsetY );

		ImVec2 localPos;
		localPos.x = static_cast<float>( rebasedEntityPosition.Get<AxisU>() * invZoom + offsetX );
		localPos.y = static_cast<float>( rebasedEntityPosition.Get<AxisV>() * invZoom + offsetY );

		// Only draw X if smaller than bounding box
		if ( kPositionHandleSize * 2 <= localBoxMax.x - localBoxMin.x && kPositionHandleSize * 2 <= localBoxMax.y - localBoxMin.y ) {
			DrawCross( drawList, localPos, kPositionHandleSize, entityColor );
		}

		if ( kInformationVirtualSize * 2 <= localBoxMax.x - localBoxMin.x && kInformationVirtualSize * 2 <= localBoxMax.y - localBoxMin.y ) {
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMin.y - ImGui::GetTextLineHeight() }, entityColor, Cyclone::Core::Entity::EntityTypeRegistry::GetEntityTypeName( entityType ) );
			drawList->AddText( narrowFont, fontSize, { localBoxMin.x, localBoxMax.y }, entityColor, std::format( "id={}", static_cast<size_t>( entity ) ).c_str() );
		}

		if ( entityInSelection ) {
			drawList->AddRect( localBoxMin, localBoxMax, entityColor, 0, 0, 2 );

			selectedBoxMin.x = std::min( selectedBoxMin.x, localBoxMin.x );
			selectedBoxMin.y = std::min( selectedBoxMin.y, localBoxMin.y );

			selectedBoxMax.x = std::max( selectedBoxMax.x, localBoxMax.x );
			selectedBoxMax.y = std::max( selectedBoxMax.y, localBoxMax.y );
		}
	}

	drawList->ChannelsMerge();

	if ( !inLevelInterface->GetSelectedEntities().empty() ) {
		//selectedBoxMin = { selectedBoxMin.x - 1, selectedBoxMin.y - 1 };
		//selectedBoxMax = { selectedBoxMax.x + 1, selectedBoxMax.y + 1 };
		drawList->AddRect( selectedBoxMin, selectedBoxMax, IM_COL32( 255, 0, 0, 255 ), 0, 0, 2 );

		for ( float x = selectedBoxMin.x; x < selectedBoxMax.x - 8; x += 16 ) {
			drawList->AddLine( { x, selectedBoxMin.y }, { x + 8, selectedBoxMin.y }, IM_COL32( 255, 255, 0, 255 ), 2 );
			drawList->AddLine( { x - 1, selectedBoxMax.y - 1 }, { x + 7, selectedBoxMax.y - 1 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}

		for ( float y = selectedBoxMin.y; y < selectedBoxMax.y - 8; y += 16 ) {
			drawList->AddLine( { selectedBoxMin.x, y }, { selectedBoxMin.x, y + 8 }, IM_COL32( 255, 255, 0, 255 ), 2 );
			drawList->AddLine( { selectedBoxMax.x - 1, y - 1 }, { selectedBoxMax.x - 1, y + 7 }, IM_COL32( 255, 255, 0, 255 ), 2 );
		}

		ImGui::SetCursorPos( { selectedBoxMin.x - viewOrigin.x, selectedBoxMin.y - viewOrigin.y } );
		ImGui::InvisibleButton( "Selection", { selectedBoxMax.x - selectedBoxMin.x, selectedBoxMax.y - selectedBoxMin.y }, ImGuiButtonFlags_MouseButtonLeft );
		const bool isSelectionHovered = ImGui::IsItemHovered();
		const bool isSelectionActive = ImGui::IsItemActive();

		if ( isSelectionActive ) {
			ImVec2 selectionMouseDrag = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left, 0.0f );
			
			ImGui::SetTooltip( "Mouse Drag: (%f, %f)", selectionMouseDrag.x, selectionMouseDrag.y );
			//OutputDebugStringA( "Clicked\n" );

			auto &&positionDeltaStorage = registry.storage<Cyclone::Core::Component::Position>( "delta"_hs );
			
			if ( !positionDeltaStorage.contains( inLevelInterface->GetSelectedEntity() ) ) {
				positionDeltaStorage.emplace( inLevelInterface->GetSelectedEntity(), registry.get<Cyclone::Core::Component::Position>( inLevelInterface->GetSelectedEntity() ) );
			}

			Cyclone::Core::Component::Position startPosition = positionDeltaStorage.get( inLevelInterface->GetSelectedEntity() );

			Cyclone::Core::Component::Position positionDelta{ XLVector::sZero() };

			if ( mGridSnapType == EGridSnapType::ByGrid ) {
				positionDelta = Cyclone::Core::Component::Position(
					XLVector::sZeroSetValueByIndex<AxisU>( std::round( -selectionMouseDrag.x * mZoomScale2D / mSubGridSize ) * mSubGridSize ) +
					XLVector::sZeroSetValueByIndex<AxisV>( std::round( -selectionMouseDrag.y * mZoomScale2D / mSubGridSize ) * mSubGridSize ) +
					startPosition -
					registry.get<Cyclone::Core::Component::Position>( inLevelInterface->GetSelectedEntity() )
				);
			}
			else if ( mGridSnapType == EGridSnapType::ToGrid ) {
				positionDelta = Cyclone::Core::Component::Position(
					XLVector::sZeroSetValueByIndex<AxisU>( std::round( -selectionMouseDrag.x * mZoomScale2D / mSubGridSize ) * mSubGridSize ) +
					XLVector::sZeroSetValueByIndex<AxisV>( std::round( -selectionMouseDrag.y * mZoomScale2D / mSubGridSize ) * mSubGridSize ) +
					startPosition -
					registry.get<Cyclone::Core::Component::Position>( inLevelInterface->GetSelectedEntity() )
				);

				positionDelta +=
					XLVector::sZeroSetValueByIndex<AxisU>( std::round( startPosition.Get<AxisU>() / mSubGridSize ) * mSubGridSize - startPosition.Get<AxisU>() ) +
					XLVector::sZeroSetValueByIndex<AxisV>( std::round( startPosition.Get<AxisV>() / mSubGridSize ) * mSubGridSize - startPosition.Get<AxisV>() );
			}
			else {
				positionDelta = Cyclone::Core::Component::Position(
					XLVector::sZeroSetValueByIndex<AxisU>( -selectionMouseDrag.x * mZoomScale2D ) +
					XLVector::sZeroSetValueByIndex<AxisV>( -selectionMouseDrag.y * mZoomScale2D ) +
					startPosition -
					registry.get<Cyclone::Core::Component::Position>( inLevelInterface->GetSelectedEntity() )
				);
			}

			for ( const entt::entity entity : inLevelInterface->GetSelectedEntities() ) {
				registry.get<Cyclone::Core::Component::Position>( entity ) += positionDelta;
			}
		}
		else if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) {
			registry.storage<Cyclone::Core::Component::Position>( "delta"_hs ).clear();
		}
	}
}

void Cyclone::UI::ViewportManager::Update( float inDeltaTime, Cyclone::Core::LevelInterface *inLevelInterface )
{
	ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

	ImVec2 perspectiveViewSize;

	if ( mShouldAutosize || ImGui::IsKeyChordPressed( ImGuiKey_A | ImGuiMod_Ctrl ) ) {
		mShouldAutosize = false;
		ImGui::SetNextWindowSize( { ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2 } );
	}

	ImGui::SetNextWindowSizeConstraints( { 64.0f, 64.0f }, { ImGui::GetContentRegionAvail().x - 64.0f, ImGui::GetContentRegionAvail().y - 64.0f } );
	if ( ImGui::BeginChild( "PerspectiveView", ImVec2( ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY, viewportFlags ) ) {
		perspectiveViewSize = ImGui::GetWindowSize();
		ImGui::Image( mViewportPerspective->GetOrResizeSRV( static_cast<size_t>( perspectiveViewSize.x ), static_cast<size_t>( perspectiveViewSize.y ) ), perspectiveViewSize );

		UpdatePerspective( inDeltaTime );

		DrawViewportOverlay( "Perspective" );
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if ( ImGui::BeginChild( "TopView", ImVec2( ImGui::GetContentRegionAvail().x, perspectiveViewSize.y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		UpdateWireframe<EViewportType::TopXZ>( inDeltaTime, inLevelInterface );

		DrawViewportOverlay( "Top (X/Z)" );
	}
	ImGui::EndChild();

	if ( ImGui::BeginChild( "FrontView", ImVec2( perspectiveViewSize.x, ImGui::GetContentRegionAvail().y ), ImGuiChildFlags_Borders, viewportFlags ) ) {
		UpdateWireframe<EViewportType::FrontXY>( inDeltaTime, inLevelInterface );

		DrawViewportOverlay( "Front (X/Y)" );
	}
	ImGui::EndChild();

	ImGui::SameLine();
	if ( ImGui::BeginChild( "SideView", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders, viewportFlags ) ) {
		UpdateWireframe<EViewportType::SideYZ>( inDeltaTime, inLevelInterface );

		DrawViewportOverlay( "Side (Y/Z)" );

	}
	ImGui::EndChild();

	mCenter2D = XLVector::sClamp( mCenter2D, XLVector::sReplicate( -mWorldLimit ), XLVector::sReplicate( mWorldLimit ) );
}

void Cyclone::UI::ViewportManager::RenderPerspective( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	constexpr EViewportType T = EViewportType::Perspective;

	ViewportElement *viewport = GetViewport<T>();

	viewport->Clear( inDeviceContext );
	
	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>(), GetProjMatrix<T>() );
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		mWireframeGridBatch->DrawLine(
			{ ( -mCenter3D ).ToXMVECTOR(), DirectX::Colors::DarkRed },
			{ ( -mCenter3D + XLVector::sZeroSetValueByIndex<0>( 1 ) ).ToXMVECTOR(), DirectX::Colors::DarkRed }
		);

		mWireframeGridBatch->DrawLine(
			{ ( -mCenter3D ).ToXMVECTOR(), DirectX::Colors::Green },
			{ ( -mCenter3D + XLVector::sZeroSetValueByIndex<1>( 1 ) ).ToXMVECTOR(), DirectX::Colors::Green }
		);

		mWireframeGridBatch->DrawLine(
			{ ( -mCenter3D ).ToXMVECTOR(), DirectX::Colors::DarkBlue },
			{ ( -mCenter3D + XLVector::sZeroSetValueByIndex<2>( 1 ) ).ToXMVECTOR(), DirectX::Colors::DarkBlue }
		);
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
				entityColorU32 = IM_COL32( 255, 255, 0, 255 );
			}
			else if ( entityInSelection ) {
				entityColorU32 = IM_COL32( 255, 128, 0, 255 );
			}
			else {
				entityColorU32 = entt::resolve( static_cast<entt::id_type>( entityType ) ).data( "debug_color"_hs ).get( {} ).cast<uint32_t>();
			}

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

			XLVector rebasedEntityPosition = ( position - mCenter3D );
			XLVector rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			DrawWireframeBoundingBox( mWireframeGridBatch.get(), rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}
	}
	mWireframeGridBatch->End();

	viewport->Resolve( inDeviceContext );
}

void Cyclone::UI::ViewportManager::RenderTop( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	RenderWireframe<EViewportType::TopXZ>( inDeviceContext, inLevelInterface );
}

void Cyclone::UI::ViewportManager::RenderFront( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	RenderWireframe<EViewportType::FrontXY>( inDeviceContext, inLevelInterface );
}

void Cyclone::UI::ViewportManager::RenderSide( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	RenderWireframe<EViewportType::SideYZ>( inDeviceContext, inLevelInterface );
}


template<Cyclone::UI::EViewportType T>
void Cyclone::UI::ViewportManager::RenderWireframe( ID3D11DeviceContext3 *inDeviceContext, const Cyclone::Core::LevelInterface *inLevelInterface )
{
	constexpr size_t AxisU = ViewportTypeTraits<T>::AxisU;
	constexpr size_t AxisV = ViewportTypeTraits<T>::AxisV;

	ViewportElement *viewport = GetViewport<T>();

	viewport->Clear( inDeviceContext );

	inDeviceContext->OMSetBlendState( mCommonStates->Opaque(), nullptr, 0xFFFFFFFF );
	inDeviceContext->OMSetDepthStencilState( mCommonStates->DepthNone(), 0 );
	inDeviceContext->RSSetState( mCommonStates->CullNone() );
	inDeviceContext->IASetInputLayout( mWireframeGridInputLayout.Get() );

	mWireframeGridEffect->SetMatrices( DirectX::XMMatrixIdentity(), GetViewMatrix<T>(), GetProjMatrix<T>());
	mWireframeGridEffect->Apply( inDeviceContext );

	mWireframeGridBatch->Begin();
	{
		double minU, maxU, minV, maxV;
		GetMinMaxUV<T>( minU, maxU, minV, maxV );

		double subgridStep = mSubGridSize;
		double gridStep = mSubGridSize * 10;

		while ( subgridStep / mZoomScale2D < mMinGridSize ) {
			subgridStep *= 10;
		}

		while ( gridStep / mZoomScale2D < mMinGridSize * 5 ) {
			gridStep *= 10;
		}

		if ( subgridStep / mZoomScale2D > mMinGridSize ) {
			DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, subgridStep, DirectX::ColorsLinear::DimGray );
			DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, subgridStep, DirectX::ColorsLinear::DimGray );
		}

		DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, gridStep, DirectX::Colors::DimGray );
		DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, gridStep, DirectX::Colors::DimGray );

		DrawLineLoop<AxisU, AxisV>( minU, maxU, minV, maxV, 1000, DirectX::Colors::Gray );
		DrawLineLoop<AxisV, AxisU>( minV, maxV, minU, maxU, 1000, DirectX::Colors::Gray );

		DrawAxisLine<AxisU>( minU, maxU );
		DrawAxisLine<AxisV>( minV, maxV );
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
				entityColorU32 = IM_COL32( 255, 255, 0, 255 );
			}
			else if ( entityInSelection ) {
				entityColorU32 = IM_COL32( 255, 128, 0, 255 );
			}
			else {
				entityColorU32 = entt::resolve( static_cast<entt::id_type>( entityType ) ).data( "debug_color"_hs ).get( {} ).cast<uint32_t>();
			}

			DirectX::XMVECTOR entityColorV = Cyclone::Util::ColorU32ToXMVECTOR( entityColorU32 );

			XLVector rebasedEntityPosition = ( position - mCenter2D );
			XLVector rebasedBoundingBoxPosition = rebasedEntityPosition + boundingBox.mCenter;

			DrawWireframeBoundingBox( mWireframeGridBatch.get(), rebasedBoundingBoxPosition.ToXMVECTOR(), boundingBox.mExtent.ToXMVECTOR(), entityColorV );
		}
	}
	mWireframeGridBatch->End();

	viewport->Resolve( inDeviceContext );
}