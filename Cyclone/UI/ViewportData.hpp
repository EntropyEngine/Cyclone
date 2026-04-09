#pragma once

namespace Cyclone::Rendering::Shader {
	class EntityIndexShader;
}

namespace Cyclone::UI
{
	struct ViewportData
	{
		ImDrawList *mDrawList;
		ImVec2 mViewOrigin;
		ImVec2 mViewSize;

		ImGuiID mCanvasID;
		bool mIsActive;

		ImVec2 mAbsoluteMouse;

		double mWorldMouseU;
		double mWorldMouseV;

		DirectX::XMMATRIX mViewMatrix;
		DirectX::XMMATRIX mProjMatrix;

		ID3D11DeviceContext *mDeviceContext;
		ID3D11ShaderResourceView *mEntitySRV;
		Cyclone::Rendering::Shader::EntityIndexShader *mEntityIndexShader;

		ImVec2 ClipToScreen( const ImVec2 &inClip ) const
		{
			return {
				inClip.x * ( mViewSize.x / 2.0f ) + ( mViewSize.x / 2.0f ) + mViewOrigin.x,
				-inClip.y * ( mViewSize.y / 2.0f ) + ( mViewSize.y / 2.0f ) + mViewOrigin.y
			};
		}

		void ClipToScreen( DirectX::XMFLOAT3 &ioClip ) const
		{
			ioClip.x = ioClip.x * ( mViewSize.x / 2.0f ) + ( mViewSize.x / 2.0f ) + mViewOrigin.x;
			ioClip.y = -ioClip.y * ( mViewSize.y / 2.0f ) + ( mViewSize.y / 2.0f ) + mViewOrigin.y;
		}
	};
}