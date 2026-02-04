#include "pch.h"

#include "Cyclone/Core/Level.hpp"

void Cyclone::Core::Level::Initialize()
{

}

void Cyclone::Core::Level::SetDevice( ID3D11Device3 *inDevice )
{
	if ( mDevice ) {
		ReleaseResources();
	}

	mDevice = inDevice;
}

void Cyclone::Core::Level::ReleaseResources()
{
	// Iterate over all components which hold DX resources and release them
	// TODO

	// Release device
	mDevice.Reset();
}