#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass::Shared
{
	typedef struct _TEXTEX {
		D2D1_COLOR_F color;
		D2D1_COLOR_F shadowcolor;
		float glowopacity = 0.f;
		DWORD tbWidth = 0;
		MARGINS textInset = { 0 };
		bool render = false;
		RECT fillboxSize = { 0 };
		bool isActive = true; // this is used mainly to check for re-rendering when window state changes
	} TEXTEX;

	inline enum class Type : UCHAR
	{
		Blur,
		Aero
	} g_type{ 0 };
	inline enum class TextAlign : UCHAR
	{
		Left,
		CenterW8,
		CenterUx,
		CenterDulappy,
		Right
	} g_textAlign{ 0 };
	inline enum class TBAlign : UCHAR
	{
		Top,
		Center,
		TBCenter,
		Bottom,
		Full
	} g_tbAlign{ 0 }, g_tbAlignPalette{ 0 };
	inline enum class TxtVertAlign : UCHAR
	{
		Center,
		TBCenter
	} g_iconTextVerticalAlign{ 0 };
	inline bool g_enableGeometryMerging{ false };
	inline bool g_disableOnBattery{ true };
	inline bool g_overrideAccent{ false };
	inline bool g_batteryMode{ false };
	inline bool g_transparencyEnabled{ true };
	inline float g_blurAmount{ 9.f };
	inline float g_glassOpacity{ 0.63f };
	// exclusively used by aero backdrop
	inline float g_colorBalance{ 0.f };
	inline float g_afterglowBalance{ 0.43f };
	inline float g_blurBalance{ 0.49f };
	// title button resizing functions
	inline float g_textShadowOffsetX = 1.0f;
	inline float g_textShadowOffsetY = 1.0f;

	inline float g_textGlowOpacityActive = 1.0f;
	inline float g_textGlowOpacityInactive = 0.4f;

	inline int g_textInset = 5;       // a static number added to the inset

	inline D2D1_COLOR_F g_color
	{
		116.f / 255.f, 
		184.f / 255.f, 
		252.f / 255.f, 
		1.f 
	};
	inline D2D1_COLOR_F g_afterglow
	{
		116.f / 255.f,
		184.f / 255.f,
		252.f / 255.f,
		1.f
	};
	inline bool g_forceAccentColorization{ false };
	inline D2D1_COLOR_F g_accentColor{};
	inline D2D1_COLOR_F g_accentColorInactive{};

	inline float g_reflectionIntensity{ 0.f };
	inline float g_reflectionParallaxIntensity{ 0.1f };

	FORCEINLINE bool IsBackdropAllowed()
	{
		if (g_batteryMode && g_disableOnBattery)
		{
			return false;
		}
		if (!g_transparencyEnabled)
		{
			return false;
		}

		return true;
	}

	inline bool g_enableFullDirty{ false };
}