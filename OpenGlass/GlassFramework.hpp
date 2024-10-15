﻿#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "ConfigurationFramework.hpp"

namespace OpenGlass::GlassFramework
{
	void UpdateConfiguration(ConfigurationFramework::UpdateType type);
	
	HRESULT Startup();
	void Shutdown();
}