#pragma once

namespace Cyclone::UI
{
	struct ViewportData
	{
		ImDrawList *mDrawList;
		ImVec2 mViewOrigin;
		ImVec2 mViewSize;

		ImGuiID mCanvasID;
		bool mIsActive;

		double mWorldMouseU;
		double mWorldMouseV;

		DirectX::XMMATRIX mViewMatrix;
		DirectX::XMMATRIX mProjMatrix;

		ImVec2 ClipToScreen( const ImVec2 &inClip ) const
		{
			return {
				inClip.x * ( mViewSize.x / 2.0f ) + ( mViewSize.x / 2.0f ) + mViewOrigin.x,
				-inClip.y * ( mViewSize.y / 2.0f ) + ( mViewSize.y / 2.0f ) + mViewOrigin.y
			};
		}
	};
}