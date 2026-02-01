#pragma once

// Cyclone includes
#include "Cyclone/UI/ViewportElement.hpp"

namespace Cyclone
{
	namespace UI
	{
		class ViewportManager
		{
		public:
			ViewportManager( ID3D11Device3 *inDevice );

			ViewportManager( ViewportManager && ) = default;
			ViewportManager &operator= ( ViewportManager && ) = default;

			ViewportManager( ViewportManager const & ) = delete;
			ViewportManager &operator= ( ViewportManager const & ) = delete;

			void Update( float inDeltaTime );

			Cyclone::UI::ViewportElement *GetPerspective() { return mViewportPerspective.get(); }
			Cyclone::UI::ViewportElement *GetTop()		   { return mViewportTop.get(); }
			Cyclone::UI::ViewportElement *GetFront()	   { return mViewportFront.get(); }
			Cyclone::UI::ViewportElement *GetSide()		   { return mViewportSide.get(); }

		protected:
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportPerspective;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportTop;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportFront;
			std::unique_ptr<Cyclone::UI::ViewportElement> mViewportSide;

			float mZoomScale2D = 0.1f; // Pixels to meters
			double mCenterX2D = 0.0;
			double mCenterY2D = 0.0;
			double mCenterZ2D = 0.0;
		};
	}
}