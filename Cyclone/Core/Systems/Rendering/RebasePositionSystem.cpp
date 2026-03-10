#include "pch.h"
#include "Cyclone/Core/Systems/Rendering/RebasePositionSystem.hpp"

// Cyclone Core
#include "Cyclone/Core/LevelInterface.hpp"

// Cyclone Components
#include "Cyclone/Core/Component/Position.hpp"

Cyclone::Core::Systems::Rendering::RebasePositionSystem::RebasePositionSystem( entt::registry &inRegistry )
{
	mStorage.bind( inRegistry );

	mStorage
		.on_construct<Cyclone::Core::Component::Position>()
		.on_update<Cyclone::Core::Component::Position>();
}

void Cyclone::Core::Systems::Rendering::RebasePositionSystem::OnUpdateEnd( Cyclone::Core::LevelInterface *inLevelInterface )
{
	entt::registry &registry = inLevelInterface->GetRegistry();
	const auto &perspectiveContext = inLevelInterface->GetPerspectiveCtx();
	const auto &orthographicContext = inLevelInterface->GetOrthographicCtx();

	for ( auto [entity, position] : mStorage.view<Cyclone::Core::Component::Position>().each() ) {
		DirectX::XMVECTOR rebasedPerspective = ( position.mValue - perspectiveContext.mCenter3D ).ToXMVECTOR();
		DirectX::XMVECTOR rebasedOrthographic = ( position.mValue - orthographicContext.mCenter2D ).ToXMVECTOR();

		registry.emplace_or_replace<Cyclone::Core::Component::RebasedPositionPerspective>( entity, rebasedPerspective );
		registry.emplace_or_replace<Cyclone::Core::Component::RebasedPositionOrthographic>( entity, rebasedOrthographic );
	}
}
