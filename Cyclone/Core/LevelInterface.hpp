#pragma once

// Cyclone utils
#include "Cyclone/Util/NonCopyable.hpp"

// Cyclone core
#include "Cyclone/Core/Level.hpp"
#include "Cyclone/Core/Entity/EntityContext.hpp"
#include "Cyclone/Core/Tool/SelectionToolContext.hpp"
#include "Cyclone/Core/Tool/SelectionTransformToolContext.hpp"

namespace Cyclone::Core
{
	class LevelInterface : public Cyclone::Util::NonCopyable
	{
	public:
		LevelInterface();

		void					Initialize();

		void					SetDevice( ID3D11Device3 *inDevice );
		void					ReleaseResources();

		const ID3D11Device3 *	GetDevice() const					{ return mDevice.Get(); }
		ID3D11Device3 *			GetDevice()							{ return mDevice.Get(); }

		const Level *			GetLevel() const					{ return mLevel.get(); }
		Level *					GetLevel()							{ return mLevel.get(); }

		const entt::registry &	GetRegistry() const					{ return mLevel->GetRegistry(); }
		entt::registry &		GetRegistry()						{ return mLevel->GetRegistry(); }

		Entity::EntityContext & GetEntityCtx()						{ return mEntityContext; }
		const Entity::EntityContext & GetEntityCtx() const			{ return mEntityContext; }

		Tool::SelectionToolContext & GetSelectionCtx()				{ return mSelectionTool; }
		const Tool::SelectionToolContext & GetSelectionCtx() const	{ return mSelectionTool; }

		Tool::SelectionTransformToolContext & GetSelectionTransformCtx() { return mSelectionTransformTool; }
		const Tool::SelectionTransformToolContext & GetSelectionTransformCtx() const { return mSelectionTransformTool; }

	protected:
		Microsoft::WRL::ComPtr<ID3D11Device3> mDevice;

		std::unique_ptr<Level>	mLevel;
		Entity::EntityContext	mEntityContext;
		Tool::SelectionToolContext mSelectionTool;
		Tool::SelectionTransformToolContext mSelectionTransformTool;
	};
}