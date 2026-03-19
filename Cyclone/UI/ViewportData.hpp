#pragma once

namespace Cyclone::UI
{
	struct ViewportData
	{
		ImDrawList *mDrawList;
		ImVec2 mViewOrigin;
		ImVec2 mViewSize;
		bool mIsActive;

		double mWorldMouseU;
		double mWorldMouseV;
	};
}