#pragma once

namespace Cyclone::Core::Event
{
	void TestMain();

	struct TestManager
	{
		entt::registry mRegistry;

		void PrintComponents();

		void UpdateEvents();
	};

	struct ComChildren;
	struct ComParent;
	struct ComTransform;

	struct ComChildren
	{
		std::set<entt::entity> mChidren;
	};

	struct ComParent
	{
		entt::entity mParent{ entt::null };
	};

	struct ComTransform
	{
		DirectX::XMVECTORF32 mPosition;
	};


	struct EvBase;
	struct EvCommit;
	struct EvUpdateParent;
	struct EvUpdateTransform;
	struct EvUpdateTransformChildren;

	struct EvBase { entt::handle mHandle; };

	struct EvCommit : public EvBase
	{
		static void sExec( const EvCommit &inEvent )
		{
			inEvent.mHandle.registry()->ctx().get<std::set<entt::entity>>( "update_queue"_hs ).insert( inEvent.mHandle.entity() );
		}
	};

	struct EvUpdateParent : public EvBase
	{
		entt::entity mNewParent;

		static void sExec( const EvUpdateParent &inEvent );
	};

	struct EvUpdateTransformChildren : public EvBase
	{
		static void sExec( const EvUpdateTransformChildren &inEvent );
	};

	struct EvUpdateTransform : public EvBase
	{
		static void sExec( const EvUpdateTransform &inEvent )
		{
			// Commit self
			inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvCommit{ inEvent.mHandle } );

			if ( ComChildren *children = inEvent.mHandle.try_get<ComChildren>(); children ) {
				for ( entt::entity child : children->mChidren ) {
					inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvUpdateTransformChildren{ entt::handle{ *inEvent.mHandle.registry(), child } } );
				}
			}
		}
	};

	inline void EvUpdateParent::sExec( const EvUpdateParent &inEvent )
	{
		// Get original parent
		entt::entity oldParent = inEvent.mHandle.get<ComParent>().mParent;

		// Remove child from old parent's children, and commit old parent if changed
		ComChildren *parentChildren = inEvent.mHandle.registry()->try_get<ComChildren>( oldParent );
		if ( parentChildren && parentChildren->mChidren.erase( inEvent.mHandle.entity() ) ) {
			inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvCommit{ entt::handle{ *inEvent.mHandle.registry(), oldParent } } );
		}

		// Add child to new parent's children, and commit new parent if changed
		ComChildren *newParentChildren = inEvent.mHandle.registry()->try_get<ComChildren>( inEvent.mNewParent );
		if ( newParentChildren && newParentChildren->mChidren.insert( inEvent.mHandle.entity() ).second ) {
			inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvCommit{ entt::handle{ *inEvent.mHandle.registry(), inEvent.mNewParent } } );
		}

		// Set new parent
		inEvent.mHandle.get<ComParent>().mParent = inEvent.mNewParent;

		// Additionally, update ourselves if we're valid (which also commits child)
		if ( newParentChildren ) {
			inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvUpdateTransformChildren{ inEvent.mHandle } );
		}
		// Otherwise commit child
		else {
			inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvCommit{ inEvent.mHandle } );
		}
	}

	inline void EvUpdateTransformChildren::sExec( const EvUpdateTransformChildren &inEvent )
	{
		entt::entity parent = inEvent.mHandle.get<ComParent>().mParent;
		DirectX::XMVECTORF32 parentTransform = inEvent.mHandle.registry()->get<ComTransform>( parent ).mPosition;
		parentTransform.f[0] += 1.0f;

		inEvent.mHandle.replace<ComTransform>( parentTransform );

		// Commit self transform
		inEvent.mHandle.registry()->ctx().get<entt::dispatcher>().enqueue( EvUpdateTransform{ inEvent.mHandle } );
	}
}