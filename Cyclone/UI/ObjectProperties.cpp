#include "pch.h"
#include "Cyclone/UI/ObjectProperties.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/Rotation.hpp"
#include "Cyclone/Core/Component/LocalBounds.hpp"
#include "Cyclone/Core/Component/Path.hpp"

// Cyclone utils
#include "Cyclone/Util/String.hpp"

using Cyclone::Math::Vector4D;
using namespace Cyclone::Core::Component;

namespace
{
	void LineSpace()
	{
		ImGui::SameLine( std::max( 160.0f, ImGui::GetWindowWidth() / 4.0f ) );
	}

	void LeafNode( const char *inLabel, float inRatio = 1.0f )
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TreeNodeEx( inLabel, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DrawLinesFull );
		ImGui::TreePop();
		LineSpace();
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x * inRatio );
	}

	bool HandleTangents( entt::registry &inRegistry, entt::entity inEntity, PathData &inPathData, PathCache &inPathCache, int inKnot, bool inIsOut )
	{
		bool dirty = false;

		PathData::ESegmentType prevSegmentType = inKnot > 0 ? inPathData.mSegmentType[inKnot - 1] : PathData::ESegmentType::Custom;
		PathData::ESegmentType nextSegmentType = inKnot + 1 < inPathData.mKnots.size() ? inPathData.mSegmentType[inKnot] : PathData::ESegmentType::Custom;

		DirectX::XMVECTOR &vec = inIsOut ? inPathData.mKnots[inKnot].mOutVec : inPathData.mKnots[inKnot].mInVec;
		DirectX::XMVECTORF32 vecData{ .v = vec };

		ImGui::BeginDisabled( ( nextSegmentType != PathData::ESegmentType::Custom && inIsOut ) || ( prevSegmentType != PathData::ESegmentType::Custom && !inIsOut ) );

		LeafNode( inIsOut ? "OutVec" : "InVec", 2.0f / 3.0f );
		ImGui::DragScalarN( inIsOut ? "##OutVec" : "##InVec", ImGuiDataType_Float, vecData.f, 3, 0.1f, nullptr, nullptr, "%.3f" );

		float len = DirectX::XMVectorGetX( DirectX::XMVector3Length( vecData ) );
		if ( ImGui::IsItemEdited() && len > 0 ) {
			vec = vecData.v;
			inPathData.UpdateTangentValue( inKnot, inIsOut );
			inPathData.ComputeAutoExtrusions( inKnot );
			inPathCache.Rebuild( inRegistry, inEntity );
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() && len > 0 ) {
			dirty = true;
		}


		ImGui::SameLine( 0.0f );
		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Len:" );
		ImGui::SameLine( 0.0f, 0.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::DragFloat( inIsOut ? "##OutVecLen" : "##InVecLen", &len, 0.1f, 0.1f, FLT_MAX );

		if ( ImGui::IsItemEdited() && len > 0 ) {
			vec = DirectX::XMVectorScale( DirectX::XMVector3Normalize( vecData.v ), std::max( 0.1f, len ) );
			inPathData.UpdateTangentValue( inKnot, inIsOut );
			inPathData.ComputeAutoExtrusions( inKnot );
			inPathCache.Rebuild( inRegistry, inEntity );
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() && len > 0 ) {
			dirty = true;
		}

		ImGui::EndDisabled();

		return dirty;
	}

	bool HandleExtrusions( entt::registry &inRegistry, entt::entity inEntity,PathData &inPathData, PathCache &inPathCache, int inKnot, bool isBitan )
	{
		bool dirty = false;

		DirectX::XMVECTOR &vec = isBitan ? inPathData.mExtrusions[inKnot].mBitangent : inPathData.mExtrusions[inKnot].mNormal;
		DirectX::XMVECTORF32 vecData{ .v = vec };

		LeafNode( isBitan ? "Bitangent" : "Normal" );

		bool isCustom = inPathData.mExtrusionTypes[inKnot] & ( isBitan ? PathData::EExtrusionType::CustomBitangent : PathData::EExtrusionType::CustomNormal );
		if ( ImGui::Checkbox( isBitan ? "##BitanCustom" : "##NormalCustom", &isCustom ) ) {
			inPathData.mExtrusionTypes[inKnot] ^= ( isBitan ? PathData::EExtrusionType::CustomBitangent : PathData::EExtrusionType::CustomNormal );
			inPathData.ComputeAutoExtrusions( inKnot, isBitan );
			inPathCache.Rebuild( inRegistry, inEntity );
			dirty = true;
		}

		ImGui::BeginDisabled( !isCustom );

		ImGui::SameLine( 0.0f, 4.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::DragScalarN( isBitan ? "##BitanVec" : "##NormalVec", ImGuiDataType_Float, vecData.f, 3, 0.1f, nullptr, nullptr, "%.5f" );

		float len = DirectX::XMVectorGetX( DirectX::XMVector3Length( vecData ) );
		if ( ImGui::IsItemEdited() && len > 0 ) {
			vec = DirectX::XMVectorScale( vecData.v, 1.0f / len );
			inPathData.ComputeAutoExtrusions( inKnot, isBitan );
			inPathCache.Rebuild( inRegistry, inEntity );
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() && len > 0 ) {
			dirty = true;
		}

		ImGui::EndDisabled();

		return dirty;
	}
}

void Cyclone::UI::ObjectProperties::ShowWindow( Cyclone::Core::LevelInterface *inLevelInterface, entt::entity inEntity )
{
	entt::registry &registry = inLevelInterface->GetRegistry();
	auto &entityManager = inLevelInterface->GetEntityManager();

	bool dirty = false;

	EntityType entityType = registry.get<EntityType>( inEntity );
	EntityCategory entityCategory = registry.get<EntityCategory>( inEntity );

	ImGui::AlignTextToFramePadding();
	ImGui::Text( "Type" );
	LineSpace();
	ImGui::Text( entityManager.GetEntityTypeName( entityType ) );

	ImGui::AlignTextToFramePadding();
	ImGui::Text( "Category" );
	LineSpace();
	ImGui::Text( entityManager.GetEntityCategoryName( entityCategory ) );

	ImGui::Separator();

	{
		Position &position = registry.get<Position>( inEntity );
		double positionData[4];
		position.mValue.Store( positionData );
		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Position" );
		LineSpace();
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::DragScalarN( "##Position", ImGuiDataType_Double, positionData, 3, 1.0f, nullptr, nullptr, "%.2f" );
		if ( ImGui::IsItemEdited() ) {
			position.mValue = Vector4D::sLoad( positionData ); // TODO: snapping?
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() ) {
			dirty = true;
		}
	}

	{
		Rotation &rotation = registry.get<Rotation>( inEntity );
		DirectX::XMVECTORF32 rotationData = { .v = rotation.mPitchYawRoll };
		rotationData.v = rotationData * ( 180.0f / DirectX::XM_PI );
		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Rotation" );
		LineSpace();
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
		ImGui::DragScalarN( "##Rotation", ImGuiDataType_Float, rotationData.f, 3, 1.0f, nullptr, nullptr, "%.2f" );
		if ( ImGui::IsItemEdited() ) {
			rotation.mPitchYawRoll = rotationData * ( DirectX::XM_PI / 180.0f );
			registry.get<LocalBounds>( inEntity ).UpdateBoundingBox( inEntity, registry );
		}
		if ( ImGui::IsItemDeactivatedAfterEdit() ) {
			dirty = true;
		}
	}

	if ( registry.all_of<PathTag>( inEntity ) ) {
		ImGui::SeparatorText( "Path Data" );

		PathData &pathData = registry.get<PathData>( inEntity );
		PathCache &pathCache = registry.get<PathCache>( inEntity );
		PathSelection &pathSelection = registry.get<PathSelection>( inEntity );

		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Knot Count" );
		LineSpace();
		ImGui::Text( Cyclone::Util::PrefixString( "", pathData.mKnots.size() ) );		

		ImGui::Separator();

		ImGuiStorage *localStorage = ImGui::GetStateStorage();

		if ( !localStorage->GetVoidPtr( ImGui::GetID( "AddLoopAngle" ) ) ) localStorage->SetFloat( ImGui::GetID( "AddLoopAngle" ), 360.0f );
		if ( !localStorage->GetVoidPtr( ImGui::GetID( "AddLoopStride" ) ) ) localStorage->SetFloat( ImGui::GetID( "AddLoopStride" ), 0.0f );
		if ( !localStorage->GetVoidPtr( ImGui::GetID( "AddLoopRadius" ) ) ) localStorage->SetFloat( ImGui::GetID( "AddLoopRadius" ), 2.0f );


		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Add Segment" );
		LineSpace();
		if ( ImGui::Button( "+##AddSegment", { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() } ) ) {
			pathData.AddKnot();
			dirty = true;
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Add Loop" );
		LineSpace();
		if ( ImGui::Button( "+##AddLoop", { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() } ) ) {
			pathData.AddFullLoop(
				localStorage->GetFloat( ImGui::GetID( "AddLoopAngle" ) ) / 180.0 * DirectX::XM_PI,
				localStorage->GetFloat( ImGui::GetID( "AddLoopStride" ) ),
				localStorage->GetFloat( ImGui::GetID( "AddLoopRadius" ) )
			);
			dirty = true;
		}
		ImGui::SameLine( 0.0f, 4.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x / 3 );
		ImGui::DragFloat( "##AddLoopAngle", localStorage->GetFloatRef( ImGui::GetID( "AddLoopAngle" ) ), 1.0f, 90.0f, 360.0f, "Angle=%.0f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat );
		ImGui::SameLine( 0.0f, 4.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x / 2 );
		ImGui::DragFloat( "##AddLoopStride", localStorage->GetFloatRef( ImGui::GetID( "AddLoopStride" ) ), 1.0f, 0.0f, 0.0f, "Stride=%.2f", ImGuiSliderFlags_NoRoundToFormat );
		ImGui::SameLine( 0.0f, 4.0f );
		ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x / 1 );
		ImGui::DragFloat( "##AddLoopRadius", localStorage->GetFloatRef( ImGui::GetID( "AddLoopRadius" ) ), 1.0f, 0.0f, 0.0f, "Radius=%.2f", ImGuiSliderFlags_NoRoundToFormat );

		ImGui::AlignTextToFramePadding();
		ImGui::Text( "Add Half-Loop" );
		LineSpace();
		if ( ImGui::Button( "+##AddHalfLoop", { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() } ) ) {
			pathData.AddHalfLoop( DirectX::XM_PI, 1.0f, 2.0f );
			dirty = true;
		}
		
		if ( ImGui::CollapsingHeader( "Knots" ) ) {

			for ( int i = 0; i < pathData.mKnots.size(); ++i ) {

				PathData::ESegmentType prevSegmentType = i > 0 ? pathData.mSegmentType[i - 1] : PathData::ESegmentType::Custom;
				PathData::ESegmentType nextSegmentType = i + 1 < pathData.mKnots.size() ? pathData.mSegmentType[i] : PathData::ESegmentType::Custom;

				ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DrawLinesFull;

				if ( pathSelection.mSelectedKnots.contains( i ) ) {
					rootFlags |= ImGuiTreeNodeFlags_Selected;
				}


				ImGui::AlignTextToFramePadding();
				bool isKnotOpen = ImGui::TreeNodeEx( Cyclone::Util::PrefixString( "Knot ", i ), rootFlags );

				if ( pathSelection.mCurrentKnot == i ) {
					ImGui::SameLine();
					ImGui::Bullet();
					ImGui::Dummy( {} );
				}

				if ( isKnotOpen ) {

					ImGui::BeginDisabled( prevSegmentType != PathData::ESegmentType::Custom );
					/* Position */ {
						Vector4D &position = pathData.mKnots[i].mPoint;
						double positionData[4];
						position.Store( positionData );

						LeafNode( "Position" );
						ImGui::DragScalarN( "##Position", ImGuiDataType_Double, positionData, 3, 0.1f, nullptr, nullptr, "%.3f" );

						if ( ImGui::IsItemEdited() ) {
							position = Vector4D::sLoad( positionData );
							pathCache.Rebuild( registry, inEntity );
						}
						if ( ImGui::IsItemDeactivatedAfterEdit() ) {
							dirty = true;
						}
					}
					ImGui::EndDisabled();

					/* Path Width */ {
						float width = pathData.mPathWidths[i];
						LeafNode( "Path Width", 1.0f / 3.0f );
						ImGui::DragFloat( "##PathWidth", &width, 0.01f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat );

						if ( ImGui::IsItemEdited() ) {
							pathData.mPathWidths[i] = width;
							pathCache.Rebuild( registry, inEntity );
						}
						if ( ImGui::IsItemDeactivatedAfterEdit() ) {
							dirty = true;
						}
					}

					/* Tangent Type */ {
						PathData::ETangentType &tangentType = pathData.mTangentType[i];
						int tidx = static_cast<int>( tangentType );

						LeafNode( "Tangent Type" );
						if ( ImGui::Combo( "##TangentType", &tidx, PathData::kTangentTypes, std::size( PathData::kTangentTypes ) ) ) {
							tangentType = static_cast<PathData::ETangentType>( tidx );
							pathData.UpdateTangentType( i, false );
							pathCache.Rebuild( registry, inEntity );
							dirty = true;
						}
					}

					/* Tangent Controls */ {
						dirty |= HandleTangents( registry, inEntity, pathData, pathCache, i, false );
						dirty |= HandleTangents( registry, inEntity, pathData, pathCache, i, true );
					}

					/* Normal Alignment */ {
						LeafNode( "Align Normal" );

						int nidx = ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::NORMAL_MASK ) - 1;
						if ( ImGui::Combo( "##AlignNormal", &nidx, PathData::kExtrusionTypes, std::size( PathData::kExtrusionTypes ) ) ) {
							pathData.mExtrusionTypes[i] &= ~PathData::EExtrusionType::NORMAL_MASK;
							pathData.mExtrusionTypes[i] |= nidx + 1;
							dirty = true;

							pathCache.Rebuild( registry, inEntity );

							// TODO: what the fuck?
						}
					}

					/* Bitangent Alignment */ {
						LeafNode( "Align Bitangent" );

						int bidx = ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::BITANGENT_MASK ) >> 2 ) - 1;
						if ( ImGui::Combo( "##AlignBitangent", &bidx, PathData::kExtrusionTypes, std::size( PathData::kExtrusionTypes ) ) ) {
							pathData.mExtrusionTypes[i] &= ~PathData::EExtrusionType::BITANGENT_MASK;
							pathData.mExtrusionTypes[i] |= ( bidx + 1 ) << 2;
							dirty = true;

							pathCache.Rebuild( registry, inEntity );
						}
					}

					/* Normal/Bitangent Controls */ {
						dirty |= HandleExtrusions( registry, inEntity, pathData, pathCache, i, false );
						dirty |= HandleExtrusions( registry, inEntity, pathData, pathCache, i, true );
					}

					// Quick Edit
					ImGui::AlignTextToFramePadding();
					if ( ImGui::TreeNodeEx( "Quick Edit", ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_DefaultOpen ) ) {
						LineSpace();
						ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x );
						if ( ImGui::BeginTable( "QuickEditTable", 3, ImGuiTableFlags_None ) ) {
							ImGui::TableNextRow();

							ImGui::TableSetColumnIndex( 0 );
							ImGui::AlignTextToFramePadding();
							ImGui::Text( "Pitch" );

							DirectX::XMVECTOR normTangent = DirectX::XMVector3Normalize( pathData.mKnots[i].mOutVec );

							float globalPitch = std::asin( DirectX::XMVectorGetX( DirectX::XMVector3Dot( normTangent, DirectX::g_XMIdentityR1 ) ) );
							float localPitch = std::asin( DirectX::XMVectorGetX( DirectX::XMVector3Dot( normTangent, pathData.mExtrusions[i].mNormal ) ) );
							ImGui::Text( "Global:%.2f", globalPitch * 180.0f / DirectX::XM_PI );
							ImGui::Text( "Local: %.2f", localPitch * 180.0f / DirectX::XM_PI );

							for ( int tn = 0; tn < 2; ++tn ) {
								float pitchDrag = 0.0f;
								ImGui::DragFloat( ( tn ? "T+N##PitchDrag" : "T##PitchDrag" ), &pitchDrag, DirectX::XM_PI / 180.0f, 0.0f, 0.0f, "", ImGuiSliderFlags_NoRoundToFormat );
								if ( ImGui::IsItemEdited() ) {
									DirectX::XMVECTOR pitchDir;
									if ( !( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CustomNormal ) ) {
										pitchDir = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( pathData.mExtrusions[i].mNormal, normTangent ) );
									}
									else if ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CustomBitangent ) ) {
										pitchDir = DirectX::XMVector3Cross( DirectX::XMVector3Normalize( DirectX::XMVector3Cross( normTangent, pathData.mExtrusions[i].mBitangent ) ), normTangent );
									}
									else {
										pitchDrag = std::clamp( localPitch + pitchDrag, -DirectX::XM_PI * 85.0f / 180.0f, DirectX::XM_PI * 85.0f / 180.0f ) - localPitch;
										pitchDir = DirectX::XMVector3Normalize( DirectX::XMVector3Cross( pathData.mExtrusions[i].mNormal, normTangent ) );
									}
									pathData.mKnots[i].mOutVec = DirectX::XMVector3Rotate( pathData.mKnots[i].mOutVec, DirectX::XMQuaternionRotationNormal( pitchDir, -pitchDrag ) );
									if ( tn ) {
										pathData.mExtrusions[i].mNormal = DirectX::XMVector3Rotate( pathData.mExtrusions[i].mNormal, DirectX::XMQuaternionRotationNormal( pitchDir, -pitchDrag ) );
									}
									pathData.UpdateTangentValue( i, true );
									pathData.ComputeAutoExtrusions( i );

									pathCache.Rebuild( registry, inEntity );
								}
								if ( ImGui::IsItemDeactivatedAfterEdit() ) {
									dirty = true;
								}
							}

							ImGui::TableSetColumnIndex( 1 );
							ImGui::AlignTextToFramePadding();
							ImGui::Text( "Yaw" );

							float localYaw = std::asin( DirectX::XMVectorGetX( DirectX::XMVector3Dot( normTangent, pathData.mExtrusions[i].mBitangent ) ) );
							ImGui::Text( "" );
							ImGui::Text( "Local:%.2f", localYaw * 180.0f / DirectX::XM_PI );

							for ( int tb = 0; tb < 2; ++tb ) {
								float yawDrag = 0.0f;
								ImGui::DragFloat( tb ? "T+B##YawDrag" : "T##YawDrag", &yawDrag, DirectX::XM_PI / 180.0f, 0.0f, 0.0f, "", ImGuiSliderFlags_NoRoundToFormat );
								if ( ImGui::IsItemEdited() ) {
									if ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CustomBitangent ) ) {
										yawDrag = std::clamp( localYaw - yawDrag, -DirectX::XM_PI * 85.0f / 180.0f, DirectX::XM_PI * 85.0f / 180.0f ) - localYaw;
									}
									pathData.mKnots[i].mOutVec = DirectX::XMVector3Rotate( pathData.mKnots[i].mOutVec, DirectX::XMQuaternionRotationNormal( pathData.mExtrusions[i].mNormal, yawDrag ) );
									if ( tb ) {
										pathData.mExtrusions[i].mBitangent = DirectX::XMVector3Rotate( pathData.mExtrusions[i].mBitangent, DirectX::XMQuaternionRotationNormal( pathData.mExtrusions[i].mNormal, yawDrag ) );
									}
									pathData.UpdateTangentValue( i, true );
									pathData.ComputeAutoExtrusions( i );

									pathCache.Rebuild( registry, inEntity );
								}
								if ( ImGui::IsItemDeactivatedAfterEdit() ) {
									dirty = true;
								}
							}

							ImGui::TableSetColumnIndex( 2 );
							ImGui::AlignTextToFramePadding();
							ImGui::Text( "Roll" );

							float normalRoll = -std::asin( DirectX::XMVectorGetX( DirectX::XMVector3Dot( pathData.mExtrusions[i].mNormal, pathData.mExtrusions[i].mBitangent ) ) );
							float bitanRoll = std::acos( DirectX::XMVectorGetX( DirectX::XMVector3Dot( pathData.mExtrusions[i].mNormal, pathData.mExtrusions[i].mBitangent ) ) );
							ImGui::Text( "LocalN:%.2f", normalRoll * 180.0f / DirectX::XM_PI );
							ImGui::Text( "LocalB:%.2f", bitanRoll * 180.0f / DirectX::XM_PI );

							{
								float rollDrag = 0.0f;
								ImGui::DragFloat( "N##RollDrag", &rollDrag, DirectX::XM_PI / 180.0f, 0.0f, 0.0f, "", ImGuiSliderFlags_NoRoundToFormat );
								if ( ImGui::IsItemEdited() ) {
									if ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CustomBitangent ) ) {
										rollDrag = std::clamp( normalRoll + rollDrag, -DirectX::XM_PI * 85.0f / 180.0f, DirectX::XM_PI * 85.0f / 180.0f ) - normalRoll;
									}
									pathData.mExtrusions[i].mNormal = DirectX::XMVector3Rotate( pathData.mExtrusions[i].mNormal, DirectX::XMQuaternionRotationNormal( normTangent, rollDrag ) );
									pathData.ComputeAutoExtrusions( i, true );

									pathCache.Rebuild( registry, inEntity );
								}
								if ( ImGui::IsItemDeactivatedAfterEdit() ) {
									dirty = true;
								}
							}

							{
								float rollDrag = 0.0f;
								ImGui::DragFloat( "B##RollDrag", &rollDrag, DirectX::XM_PI / 180.0f, 0.0f, 0.0f, "", ImGuiSliderFlags_NoRoundToFormat );
								if ( ImGui::IsItemEdited() ) {
									if ( ( pathData.mExtrusionTypes[i] & PathData::EExtrusionType::CustomNormal ) ) {
										rollDrag = std::clamp( bitanRoll - rollDrag, DirectX::XM_PI * 5.0f / 180.0f, DirectX::XM_PI * 175.0f / 180.0f ) - bitanRoll;
									}
									else {
										rollDrag = -rollDrag;
									}
									pathData.mExtrusions[i].mBitangent = DirectX::XMVector3Rotate( pathData.mExtrusions[i].mBitangent, DirectX::XMQuaternionRotationNormal( normTangent, -rollDrag ) );
									pathData.ComputeAutoExtrusions( i, false );

									pathCache.Rebuild( registry, inEntity );
								}
								if ( ImGui::IsItemDeactivatedAfterEdit() ) {
									dirty = true;
								}
							}
						}
						ImGui::EndTable();

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

		}

		if ( ImGui::CollapsingHeader( "Segments" ) ) {
			for ( int i = 0; i < pathData.mSegmentType.size(); ++i ) {

				PathData::ESegmentType prevSegmentType = i > 0 ? pathData.mSegmentType[i - 1] : PathData::ESegmentType::Custom;
				PathData::ESegmentType segmentType = pathData.mSegmentType[i];

				const char *segmentTypeName = PathData::kSegmentTypes[static_cast<size_t>( segmentType )];

				int segmentLength = 1;
				for ( ; segmentLength + i < pathData.mSegmentType.size(); ++segmentLength ) {
					if ( pathData.mSegmentType[segmentLength + i] != PathData::ESegmentType::Child ) break;
				}

				ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DrawLinesFull;

				ImGui::AlignTextToFramePadding();
				bool isSegmentOpen = ImGui::TreeNodeEx( Cyclone::Util::PrefixString( "Segment ", i ), rootFlags, "%s (%d-%d)", segmentTypeName, i, i + segmentLength );

				if ( isSegmentOpen ) {
					if ( segmentType == PathData::ESegmentType::FullLoop ) {
						assert( segmentLength == 4 );
						size_t knot0 = i;
						size_t knot1 = i + 1;
						size_t knot2 = i + 2;
						size_t knot4 = i + 4;

						Vector4D bitangent = Vector4D::sFromXMVECTOR( pathData.mExtrusions[knot0].mBitangent );
						Vector4D normal = Vector4D::sFromXMVECTOR( pathData.mExtrusions[knot0].mNormal );

						Vector4D pointDelta = pathData.mKnots[knot4].mPoint - pathData.mKnots[knot0].mPoint;
						double dx = pointDelta.Dot3( bitangent );

						double halfACos = Vector4D::sFromXMVECTOR( pathData.mExtrusions[knot0].mNormal ).Dot3( Vector4D::sFromXMVECTOR( pathData.mExtrusions[knot2].mNormal ) );
						halfACos = std::clamp( halfACos, -1.0, 1.0 );

						Vector4D widthDelta = pathData.mKnots[knot2].mPoint - pathData.mKnots[knot0].mPoint;
						double dw = ( widthDelta - Vector4D::sReplicate( widthDelta.Dot3( bitangent ) ) * bitangent ).GetLength3();

						Vector4D heightHalfway = ( pathData.mKnots[knot2].mPoint + pathData.mKnots[knot0].mPoint ) * Vector4D::sReplicate( 0.5 ) - pathData.mKnots[knot1].mPoint;
						double dh = ( heightHalfway - Vector4D::sReplicate( heightHalfway.Dot3( bitangent ) ) * bitangent ).GetLength3();

						double radius = dh * 0.5 + dw * dw / ( 8.0 * dh );

						if ( normal.Dot3( pointDelta ) < 0.0 ) {
							radius = -radius;
						}

						LeafNode( "Stride Length" );
						ImGui::Text( "%f", dx );

						LeafNode( "Angle" );
						ImGui::Text( "%f", std::acos( halfACos ) / DirectX::XM_PI * 360.0 );

						LeafNode( "Radius" );
						ImGui::Text( "%f", radius );

					}

					ImGui::TreePop();
				}

				i += segmentLength - 1;
			}
		}

		/*
		ImGuiStorage *localStorage = ImGui::GetStateStorage();

		if ( !localStorage->GetVoidPtr( ImGui::GetID( "test" ) ) ) {
			localStorage->SetFloat( ImGui::GetID( "test" ), 1.0f );
		}

		ImGui::DragFloat( "TestStorage", localStorage->GetFloatRef( ImGui::GetID( "test" ) ) );
		*/

		if ( dirty ) {
			registry.get<PathCache>( inEntity ).Rebuild( registry, inEntity );
		}
	}


	if ( dirty ) {
		registry.get<LocalBounds>( inEntity ).UpdateBoundingBox( inEntity, registry );
		entityManager.BeginAction();
		entityManager.UpdateEntity( inEntity, registry );
		entityManager.EndAction( registry );
	}
}
