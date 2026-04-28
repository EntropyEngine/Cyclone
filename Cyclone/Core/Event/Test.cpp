#include "pch.h"
#include "Cyclone/Core/Event/Test.hpp"

// STL Includes
#include <format>

void Cyclone::Core::Event::TestMain()
{
	OutputDebugStringA( "\n=== Start Event Test === \n\n" );

	TestManager manager;
	entt::registry &registry = manager.mRegistry;

	registry.ctx().emplace<entt::dispatcher>();
	OutputDebugStringA( "Added dispatcher to ctx\n" );

	registry.ctx().emplace_as<std::set<entt::entity>>( "update_queue"_hs );
	OutputDebugStringA( "Added update_queue to ctx\n" );

	{
		registry.ctx().get<entt::dispatcher>().sink<EvCommit>().connect<&EvCommit::sExec>();
		registry.ctx().get<entt::dispatcher>().sink<EvUpdateParent>().connect<&EvUpdateParent::sExec>();
		registry.ctx().get<entt::dispatcher>().sink<EvUpdateTransform>().connect<&EvUpdateTransform::sExec>();
		registry.ctx().get<entt::dispatcher>().sink<EvUpdateTransformChildren>().connect<&EvUpdateTransformChildren::sExec>();
		OutputDebugStringA( "Registered all event listeners\n\n" );
	}

	{
		entt::entity parentEntity = registry.create();
		registry.emplace<ComTransform>( parentEntity, 0.0f, 0.0f, 0.0f, 0.0f );
		registry.emplace<ComChildren>( parentEntity );
		OutputDebugStringF( "Added parent id={}\n", static_cast<entt::id_type>( parentEntity ) );

		for ( int i = 0; i < 3; ++i ) {
			entt::entity entity = registry.create();
			registry.emplace<ComTransform>( entity, 0.0f, 0.0f, 0.0f, 0.0f );
			registry.emplace<ComParent>( entity );
			OutputDebugStringF( "Added child id={}\n", static_cast<entt::id_type>( entity ) );
		}
	}

	OutputDebugStringA( "\n" );
	manager.PrintComponents();
	OutputDebugStringA( "\n" );


	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateParent{ entt::handle{ registry, static_cast<entt::entity>( 1 ) }, static_cast<entt::entity>( 0 ) } );
	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateParent{ entt::handle{ registry, static_cast<entt::entity>( 2 ) }, static_cast<entt::entity>( 0 ) } );
	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateParent{ entt::handle{ registry, static_cast<entt::entity>( 3 ) }, static_cast<entt::entity>( 0 ) } );
	manager.UpdateEvents();
	OutputDebugStringA( "\n" );

	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateParent{ entt::handle{ registry, static_cast<entt::entity>( 3 ) }, entt::null } );
	manager.UpdateEvents();
	OutputDebugStringA( "\n" );

	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateTransform{ entt::handle{ registry, static_cast<entt::entity>( 0 ) } } );
	manager.UpdateEvents();
	OutputDebugStringA( "\n" );

	registry.emplace<ComChildren>( static_cast<entt::entity>( 1 ) );
	manager.PrintComponents();
	OutputDebugStringA( "\n" );

	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateParent{ entt::handle{ registry, static_cast<entt::entity>( 3 ) }, static_cast<entt::entity>( 1 ) } );
	manager.UpdateEvents();
	OutputDebugStringA( "\n" );

	registry.replace<ComTransform>( static_cast<entt::entity>( 0 ), 0.0f, 1.0f, 0.0f, 0.0f );
	registry.ctx().get<entt::dispatcher>().enqueue( EvUpdateTransform{ entt::handle{ registry, static_cast<entt::entity>( 0 ) } } );
	manager.UpdateEvents();
	OutputDebugStringA( "\n" );

	OutputDebugStringA( "\n===  End Event Test  === \n\n" );
}

void Cyclone::Core::Event::TestManager::PrintComponents()
{
	for ( entt::entity entity : mRegistry.view<entt::entity>() ) {
		OutputDebugStringF( "Entity: {}", static_cast<entt::id_type>( entity ) );

		if ( ComTransform *transform = mRegistry.try_get<ComTransform>( entity ); transform ) {
			OutputDebugStringF( ", transform=[{:.1f}, {:.1f}, {:.1f}]", transform->mPosition.f[0], transform->mPosition.f[1], transform->mPosition.f[2] );
		}

		if ( ComChildren *children = mRegistry.try_get<ComChildren>( entity ); children ) {
			OutputDebugStringA( ", children=[" );

			auto it = children->mChidren.begin();
			for ( size_t i = 0; i < children->mChidren.size(); ++i, ++it ) {
				if ( i > 0 ) OutputDebugStringA( ", " );
				OutputDebugStringF( "{}", static_cast<entt::id_type>( *it ) );
			}
			OutputDebugStringA( "]" );
		}

		if ( ComParent *parent = mRegistry.try_get<ComParent>( entity ); parent ) {
			OutputDebugStringA( ", parent=" );
			if ( parent->mParent == entt::null ) {
				OutputDebugStringA( "null" );
			}
			else {
				OutputDebugStringF( "{}", static_cast<entt::id_type>( parent->mParent ) );
			}
		}

		OutputDebugStringA( "\n" );
	}
}

void Cyclone::Core::Event::TestManager::UpdateEvents()
{
	while ( size_t nEvents = mRegistry.ctx().get<entt::dispatcher>().size() ) {
		OutputDebugStringF( "Updating {} events\n", nEvents );
		mRegistry.ctx().get<entt::dispatcher>().update();
	}

	auto &updateQueue = mRegistry.ctx().get<std::set<entt::entity>>( "update_queue"_hs );
	if ( updateQueue.size() ) {
		OutputDebugStringA( "Committing entities: " );

		auto it = updateQueue.begin();
		for ( size_t i = 0; i < updateQueue.size(); ++i, ++it ) {
			if ( i > 0 ) OutputDebugStringA( ", " );
			OutputDebugStringF( "{}", static_cast<entt::id_type>( *it ) );
		}

		updateQueue.clear();

		OutputDebugStringA( "\n" );
	}

	PrintComponents();
}
