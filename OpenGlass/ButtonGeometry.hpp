#pragma once
#include "framework.hpp"
#include "ConfigurationFramework.hpp"

namespace OpenGlass::ButtonGeometry
{
	void UpdateConfiguration(ConfigurationFramework::UpdateType type);

	HRESULT Startup();
	void Shutdown();
}